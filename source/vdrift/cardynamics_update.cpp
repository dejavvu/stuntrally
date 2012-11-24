#include "pch.h"
#include "cardynamics.h"
#include "collision_world.h"
#include "settings.h"
#include "tobullet.h"
#include "../ogre/OgreGame.h"
#include "Buoyancy.h"
#include "../ogre/common/Defines.h"
#include "../ogre/common/SceneXml.h"


// executed as last function(after integration) in bullet singlestepsimulation
void CARDYNAMICS::updateAction(btCollisionWorld * collisionWorld, btScalar dt)
{
	SynchronizeBody();  // get velocity, position orientation after dt

	UpdateWheelContacts();  // update wheel contacts given new velocity, position

	Tick(dt);  // run internal simulation

	SynchronizeChassis();  // update velocity
}

void CARDYNAMICS::Update()
{
	if (!chassis)  return;//
	btTransform tr;
	chassis->getMotionState()->getWorldTransform(tr);
	chassisRotation = ToMathQuaternion<Dbl>(tr.getRotation());
	chassisCenterOfMass = ToMathVector<Dbl>(tr.getOrigin());
	MATHVECTOR <Dbl, 3> com = center_of_mass;
	chassisRotation.RotateVector(com);
	chassisPosition = chassisCenterOfMass - com;
	
	UpdateBuoyancy();
}

///................................................ Buoyancy ................................................
void CARDYNAMICS::UpdateBuoyancy()
{
	if (!pScene || (pScene->fluids.size() == 0) || !poly || !pFluids)  return;

	//float bc = /*sinf(chassisPosition[0]*20.3f)*cosf(chassisPosition[1]*30.4f) +*/
	//	sinf(chassisPosition[0]*0.3f)*cosf(chassisPosition[1]*0.32f);
	//LogO("pos " + toStr((float)chassisPosition[0]) + " " + toStr((float)chassisPosition[1]) + "  b " + toStr(bc));

	for (std::list<FluidBox*>::const_iterator i = inFluids.begin();
		i != inFluids.end(); ++i)  // 0 or 1 is there
	{
		const FluidBox* fb = *i;
		if (fb->id >= 0)
		{
			const FluidParams& fp = pFluids->fls[fb->id];

			WaterVolume water;
			//float bump = 1.f + 0.7f * sinf(chassisPosition[0]*fp.bumpFqX)*cosf(chassisPosition[1]*fp.bumpFqY);
			water.density = fp.density /* (1.f + 0.7f * bc)*/;  water.angularDrag = fp.angularDrag;
			water.linearDrag = fp.linearDrag;  water.linearDrag2 = 0.f;  //1.4f;//fp.linearDrag2;
			water.velocity.SetZero();
			water.plane.offset = fb->pos.y;  water.plane.normal = Vec3(0,0,1);
			//todo: fluid boxes rotation yaw, pitch ?-

			RigidBody body;  body.mass = body_mass;
			body.inertia = Vec3(body_inertia.getX(),body_inertia.getY(),body_inertia.getZ());

			///  body initial conditions
			//  pos & rot
			body.x.x = chassisPosition[0];  body.x.y = chassisPosition[1];  body.x.z = chassisPosition[2];
			body.q.x = chassisRotation[0];  body.q.y = chassisRotation[1];  body.q.z = chassisRotation[2];  body.q.w = chassisRotation[3];
			body.q.Normalize();//
			//  vel, ang vel
			btVector3 v = chassis->getLinearVelocity();
			btVector3 a = chassis->getAngularVelocity();
			body.v.x = v.getX();  body.v.y = v.getY();  body.v.z = v.getZ();
			body.omega.x = a.getX();  body.omega.y = a.getY();  body.omega.z = a.getZ();
			body.F.SetZero();  body.T.SetZero();
			
			//  damp from height vel
			body.F.z += fp.heightVelRes * -1000.f * body.v.z;
			
			///  add buoyancy force
			if (ComputeBuoyancy(body, *poly, water, 9.8f))
			{
				chassis->applyCentralForce( btVector3(body.F.x,body.F.y,body.F.z) );
				chassis->applyTorque(       btVector3(body.T.x,body.T.y,body.T.z) );
			}	
		}
	}

	///  wheel spin force (for mud)
	//_______________________________________________________
	for (int w=0; w < 4; ++w)
	{
		if (inFluidsWh[w].size() > 0)  // 0 or 1 is there
		{
			MATHVECTOR <Dbl, 3> up(0,0,1);
			Orientation().RotateVector(up);
			float upZ = std::max(0.f, (float)up[2]);
			
			const FluidBox* fb = *inFluidsWh[w].begin();
			if (fb->id >= 0)
			{
				const FluidParams& fp = pFluids->fls[fb->id];

				WHEEL_POSITION wp = WHEEL_POSITION(w);
				float whR = GetWheel(wp).GetRadius() * 1.2f;  //bigger par
				MATHVECTOR <float, 3> wheelpos = GetWheelPosition(wp, 0);
				wheelpos[2] -= whR;
				whP[w] = fp.idParticles;
				
				//  height in fluid:  0 just touching surface, 1 fully in fluid
				//  wheel plane distance  water.plane.normal.z = 1  water.plane.offset = fl.pos.y;
				whH[w] = (wheelpos[2] - fb->pos.y) * -0.5f / whR;
				whH[w] = std::max(0.f, std::min(1.f, whH[w]));

				if (fp.bWhForce)
				{
					//bool inAir = GetWheelContact(wp).col == NULL;

					//  bump, adds some noise
					MATHVECTOR <Dbl, 3> whPos = GetWheelPosition(wp) - chassisPosition;
					float bump = sinf(whPos[0]*fp.bumpFqX)*cosf(whPos[1]*fp.bumpFqY);
					
					float f = std::min(fp.whMaxAngVel, std::max(-fp.whMaxAngVel, (float)wheel[w].GetAngularVelocity() ));
					QUATERNION <Dbl> steer;
					float angle = -wheel[wp].GetSteerAngle() * fp.whSteerMul  + bump * fp.bumpAng;
					steer.Rotate(angle * PI_d/180.f, 0, 0, 1);

					//  forwards, side, up
					MATHVECTOR <Dbl, 3> force(whH[w] * fp.whForceLong * f, 0, /*^ 0*/100.f * whH[w] * fp.whForceUp * upZ);
					(Orientation()*steer).RotateVector(force);
					
					//  wheel spin resistance
					wheel[w].fluidRes = whH[w] * fp.whSpinDamp  * (1.f + bump * fp.bumpAmp);
					
					if (whH[w] > 0.01f /*&& inAir*/)
						chassis->applyForce( ToBulletVector(force), ToBulletVector(whPos) );
				}
			}
		}
		else
		{	whH[w] = 0.f;  wheel[w].fluidRes = 0.f;  whP[w] = -1;	}
	}

}

/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
///..........................................................................................................
void CARDYNAMICS::DebugPrint ( std::ostream & out, bool p1, bool p2, bool p3, bool p4 )
{
	using namespace std;
	out.precision(2);  out.width(6);  out << fixed;
	int cnt = pSet->car_dbgtxtcnt;

	if (p1)
	{
		#if 0  //  bullet hit data-
			out << "hit S : " << fSndForce << endl;
			out << "hit P : " << fParIntens << endl;
			//out << "hit t : " << fHitTime << endl;
			out << "bHitS : " << (bHitSnd?1:0) << " id "<< sndHitN << endl;
			out << "N Vel : " << fNormVel << endl;
			out << "v Vel : " << GetSpeed() << endl;
		#endif

		//  body
		{
			//out << "---Body---" << endl;  // L| front+back-  W_ left-right+  H/ up+down-
			out << "com: W right+ " << -center_of_mass[1] << " L front+ " << center_of_mass[0] << " H up+ " << center_of_mass[2] << endl;
			out.precision(0);
			out << "mass: " << body.GetMass() << endl;
			MATRIX3 <Dbl> inertia = body.GetInertiaConst();
			out << "inertia: roll " << inertia[0] << " pitch " << inertia[4] << " yaw " << inertia[8] << endl;
			//out << "inertia: " << inertia[0] << " " << inertia[4] << " " << inertia[8] << " < " << inertia[1] << " " << inertia[2] << " " << inertia[3] << " " << inertia[5] << " " << inertia[6] << " " << inertia[7] << endl;
			//out << "pos: " << chassisPosition << endl;
			//out << "sumWhTest: " << sumWhTest << endl;
			out.precision(2);
			//MATHVECTOR <Dbl, 3> up(0,0,1);  Orientation().RotateVector(up);
			//out << "up: " << up << endl;
			out << endl;
		}

		//  fluids
		if (cnt > 2)
		{	out << "in fluids: " << inFluids.size() <<
					" wh: " << inFluidsWh[0].size() << inFluidsWh[1].size() << inFluidsWh[2].size() << inFluidsWh[3].size() << endl;
			out << "wh fl H: " << fToStr(whH[0],1,3) << " " << fToStr(whH[1],1,3) << " " << fToStr(whH[2],1,3) << " " << fToStr(whH[3],1,3) << " \n\n";
		}

		if (cnt > 3)
		{
			engine.DebugPrint(out);  out << endl;
			//fuel_tank.DebugPrint(out);  out << endl;  //mass 8- for 3S,ES,FM
			clutch.DebugPrint(out);  out << endl;
			transmission.DebugPrint(out);	out << endl;
		}

		if (cnt > 4)
		{
			out << "---Differential---\n";
			if (drive == RWD)  {
				out << " rear\n";		diff_rear.DebugPrint(out);	}
			else if (drive == FWD)  {
				out << " front\n";		diff_front.DebugPrint(out);	}
			else if (drive == AWD)  {
				out << " center\n";		diff_center.DebugPrint(out);
				out << " front\n";		diff_front.DebugPrint(out);
				out << " rear\n";		diff_rear.DebugPrint(out);	}
			out << endl;
		}
	}

	if (p2)
	{
		out << "\n\n\n\n";
		if (cnt > 5)
		{
			out << "---Brake---\n";
			out << " FL [^" << endl;		brake[FRONT_LEFT].DebugPrint(out);
			out << " FR ^]" << endl;		brake[FRONT_RIGHT].DebugPrint(out);
			out << " RL [_" << endl;		brake[REAR_LEFT].DebugPrint(out);
			out << " RR _]" << endl;		brake[REAR_RIGHT].DebugPrint(out);
		}
		if (cnt > 7)
		{
			out << "\n---Suspension---\n";
			out << " FL [^" << endl;		suspension[FRONT_LEFT].DebugPrint(out);
			out << " FR ^]" << endl;		suspension[FRONT_RIGHT].DebugPrint(out);
			out << " RL [_" << endl;		suspension[REAR_LEFT].DebugPrint(out);
			out << " RR _]" << endl;		suspension[REAR_RIGHT].DebugPrint(out);
		}
	}

	if (p3)
		if (cnt > 6)
		{
			out << "---Wheel---\n";
			out << " FL [^" << endl;		wheel[FRONT_LEFT].DebugPrint(out);
			out << " FR ^]" << endl;		wheel[FRONT_RIGHT].DebugPrint(out);
			out << " RL [_" << endl;		wheel[REAR_LEFT].DebugPrint(out);
			out << " RR _]" << endl;		wheel[REAR_RIGHT].DebugPrint(out);
			out << "\n---Tire---\n";
			out << " FL [^" << endl;		tire[FRONT_LEFT].DebugPrint(out);
			out << " FR ^]" << endl;		tire[FRONT_RIGHT].DebugPrint(out);
			out << " RL [_" << endl;		tire[REAR_LEFT].DebugPrint(out);
			out << " RR _]" << endl;		tire[REAR_RIGHT].DebugPrint(out);
		}

	if (p4)
		if (cnt > 1)
		{
			out << "---Aerodynamic---\n";
			for (vector <CARAERO>::iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i)
				i->DebugPrint(out);
		}
}
///..........................................................................................................


void CARDYNAMICS::UpdateBody(Dbl dt, Dbl drive_torque[])
{
	body.Integrate1(dt);
	//chassis->clearForces();

	UpdateWheelVelocity();

	ApplyEngineTorqueToBody();

	ApplyAerodynamicsToBody(dt);
	

	///***  wind ~->
	if (pScene && pScene->windAmt > 0.01f)
	{
		float f = body.GetMass()*pScene->windAmt;
			// simple modulation
			float n = 1.f + 0.3f * sin(time*4.3f)*cosf(time*7.74f);
			time += dt;
		//LogO(fToStr(n,4,6));
		MATHVECTOR <Dbl, 3> v(-f*n,0,0);  // todo yaw, dir
		ApplyForce(v);
	}

	///***  manual car flip over  ---------------------------------------
	if ((doFlip > 0.01f || doFlip < -0.01f) &&
		pSet->game.flip_type > 0)
	{
		MATRIX3 <Dbl> inertia = body.GetInertia();
		btVector3 inrt(inertia[0], inertia[4], inertia[8]);
		float t = inrt[inrt.maxAxis()] * doFlip * 12.f;  // strength

		if (pSet->game.flip_type == 1)  // fuel dec
		{
			boostFuel -= doFlip > 0.f ? doFlip * dt : -doFlip * dt;
			if (boostFuel < 0.f)  boostFuel = 0.f;
			if (boostFuel <= 0.f)  t = 0.0;
		}
		MATHVECTOR <Dbl, 3> v(t,0,0);
		Orientation().RotateVector(v);
		ApplyTorque(v);
	}

	///***  boost  ------------------------------------------------------
	if (doBoost > 0.01f	&& pSet->game.boost_type > 0)
	{
		boostVal = doBoost;
		if (pSet->game.boost_type == 1 || pSet->game.boost_type == 2)  // fuel dec
		{
			boostFuel -= doBoost * dt;
			if (boostFuel < 0.f)  boostFuel = 0.f;
			if (boostFuel <= 0.f)  boostVal = 0.f;
		}
		if (boostVal > 0.01f)
		{
			float f = body.GetMass() * boostVal * 16.f * pSet->game.boost_power;  // power
			MATHVECTOR <Dbl, 3> v(f,0,0);
			Orientation().RotateVector(v);
			ApplyForce(v);
		}
	}else
		boostVal = 0.f;
		
	//  add fuel over time
	if (pSet->game.boost_type == 2)
	{
		boostFuel += dt * gfBoostFuelAddSec;
		if (boostFuel > gfBoostFuelMax)  boostFuel = gfBoostFuelMax;
	}
	//LogO(toStr(boostFuel));
	///***  -------------------------------------------------------------
	

	Dbl normal_force[WHEEL_POSITION_SIZE];
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		MATHVECTOR <Dbl, 3> suspension_force = UpdateSuspension(i, dt);
		normal_force[i] = suspension_force.dot(wheel_contact[i].GetNormal());
		if (normal_force[i] < 0) normal_force[i] = 0;

		MATHVECTOR <Dbl, 3> tire_friction = ApplyTireForce(i, normal_force[i], wheel_orientation[i]);
		ApplyWheelTorque(dt, drive_torque[i], i, tire_friction, wheel_orientation[i]);
	}

	///  test steer wheels ang vel to body yaw torque ..?  L = I*w
	//Dbl sum = 0.0;
	//for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	//{
	//	sum += (i < 2 ? -1 : 1) *//wheel[i].GetAngularVelocity() *
	//		(1.0 - sin(wheel[i].GetSteerAngle()*PI_d/180.0) ) * /*(wheel[i].GetSteerAngle() > 0 ? 1 : -1) **/ 10;
	//	MATHVECTOR <Dbl, 3> torque(0, 0, sum);
	//	//wheel_space.RotateVector(world_wheel_torque);
	//	//ApplyTorque(torque);
	//}
	//sumWhTest = sum;

	body.Integrate2(dt);
	//chassis->integrateVelocities(dt);

	// update wheel state
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_position[i] = GetWheelPositionAtDisplacement(WHEEL_POSITION(i), suspension[i].GetDisplacementPercent());
		wheel_orientation[i] = Orientation() * GetWheelSteeringAndSuspensionOrientation(WHEEL_POSITION(i));
	}
	InterpolateWheelContacts(dt);

	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		if (abs)  DoABS(i, normal_force[i]);
		if (tcs)  DoTCS(i, normal_force[i]);
	}
}

//  Tick
//---------------------------------------------------------------------------------
void CARDYNAMICS::Tick(Dbl dt)
{
	// has to happen before UpdateDriveline, overrides clutch, throttle
	UpdateTransmission(dt);

	const int num_repeats = pSet->dyn_iter;  ///~ 30+  o:10
	const float internal_dt = dt / num_repeats;
	for(int i = 0; i < num_repeats; ++i)
	{
		Dbl drive_torque[WHEEL_POSITION_SIZE];

		UpdateDriveline(internal_dt, drive_torque);

		UpdateBody(internal_dt, drive_torque);

		feedback += 0.5 * (tire[FRONT_LEFT].GetFeedback() + tire[FRONT_RIGHT].GetFeedback());
	}

	feedback /= (num_repeats + 1);

	fuel_tank.Consume(engine.FuelRate() * dt);
	//engine.SetOutOfGas(fuel_tank.Empty());
	
	if (fHitTime > 0.f)
		fHitTime -= dt * 2.f;

	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);

	UpdateTelemetry(dt);
}

void CARDYNAMICS::SynchronizeBody()
{
	MATHVECTOR<Dbl, 3> v = ToMathVector<Dbl>(chassis->getLinearVelocity());
	MATHVECTOR<Dbl, 3> w = ToMathVector<Dbl>(chassis->getAngularVelocity());
	MATHVECTOR<Dbl, 3> p = ToMathVector<Dbl>(chassis->getCenterOfMassPosition());
	QUATERNION<Dbl> q = ToMathQuaternion<Dbl>(chassis->getOrientation());
	body.SetPosition(p);
	body.SetOrientation(q);
	body.SetVelocity(v);
	body.SetAngularVelocity(w);
}

void CARDYNAMICS::SynchronizeChassis()
{
	chassis->setLinearVelocity(ToBulletVector(body.GetVelocity()));
	chassis->setAngularVelocity(ToBulletVector(body.GetAngularVelocity()));
}

void CARDYNAMICS::UpdateWheelContacts()
{
	MATHVECTOR <float, 3> raydir = GetDownVector();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		COLLISION_CONTACT & wheelContact = wheel_contact[WHEEL_POSITION(i)];
		MATHVECTOR <float, 3> raystart = LocalToWorld(wheel[i].GetExtendedPosition());
		raystart = raystart - raydir * wheel[i].GetRadius();  //*!
		float raylen = 1;  // !par
		
		//vRayStarts[i] = raystart;  // info
		//vRayDirs[i] = raystart + raydir * raylen;
		
		world->CastRay(raystart, raydir, raylen, chassis, wheelContact, /*R+*/&bWhOnRoad[i], !pSet->game.collis_cars, true);
		if (bTerrain)  ///  terrain surf from blendmap
			wheelContact.SetSurface(terSurf[i]);
	}
}
