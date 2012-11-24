#ifndef _CARAERO_H
#define _CARAERO_H

#include "dbl.h"
#include "mathvector.h"
#include "joeserialize.h"
#include "macros.h"
#include "../ogre/common/Defines.h"


class CARAERO
{
	friend class joeserialize::Serializer;
	private:
		//constants (not actually declared as const because they can be changed after object creation)
		Dbl air_density; ///the current air density in kg/m^3
		Dbl drag_frontal_area; ///the projected frontal area in m^2, used for drag calculations
		Dbl drag_coefficient; ///the drag coefficient, a unitless measure of aerodynamic drag
		Dbl lift_surface_area; ///the wing surface area in m^2
		Dbl lift_coefficient; ///a unitless lift coefficient
		Dbl lift_efficiency; ///the efficiency of the wing, a unitless value from 0.0 to 1.0
		MATHVECTOR <Dbl, 3> position; ///the position that the drag and lift forces are applied on the body
		
		//variables

		
		//for info only
		mutable MATHVECTOR <Dbl, 3> lift_vector;
		mutable MATHVECTOR <Dbl, 3> drag_vector;
		
		
	public:
		//default constructor makes an aerodynamically transparent device (i.e. no drag or lift)
		CARAERO() : air_density(1.2), drag_frontal_area(0), drag_coefficient(0),
			lift_surface_area(0), lift_coefficient(0), lift_efficiency(0) {}

		void DebugPrint(std::ostream & out)
		{
			out << "---" << std::endl;
			out << "Drag " << fToStr(drag_vector[0], 0,4) <<" "<< fToStr(drag_vector[1], 0,4) <<" "<< fToStr(drag_vector[2], 0,5) << std::endl;
			out << "Lift " << fToStr(lift_vector[0], 0,4) <<" "<< fToStr(lift_vector[1], 0,4) <<" "<< fToStr(lift_vector[2], 0,5) << std::endl;
		}
		
		void Set(const MATHVECTOR <Dbl, 3> & newpos, Dbl new_drag_frontal_area, Dbl new_drag_coefficient, Dbl new_lift_surface_area,
			 Dbl new_lift_coefficient, Dbl new_lift_efficiency)
		{
			position = newpos;
			drag_frontal_area = new_drag_frontal_area;
			drag_coefficient = new_drag_coefficient;
			lift_surface_area = new_lift_surface_area;
 			lift_coefficient = new_lift_coefficient;
			lift_efficiency = new_lift_efficiency;
		}

		const MATHVECTOR< Dbl, 3 > & GetPosition() const
		{
			return position;
		}
	
		MATHVECTOR <Dbl, 3> GetForce(const MATHVECTOR <Dbl, 3> & bodyspace_wind_vector) const
		{
			//calculate drag force
			drag_vector = bodyspace_wind_vector * bodyspace_wind_vector.Magnitude() * 0.5 *
					air_density * drag_coefficient * drag_frontal_area;
			
			//calculate lift force and associated drag
			Dbl wind_speed = -bodyspace_wind_vector[0]; //positive wind speed when the wind is heading at us
			if (wind_speed < 0)
				wind_speed = -wind_speed * 0.2; //assume the surface doesn't generate much lift when in reverse
			const Dbl k = 0.5 * air_density * wind_speed * wind_speed;
			const Dbl lift = k * lift_coefficient * lift_surface_area;
			const Dbl drag = -lift_coefficient * lift * (1.0 -  lift_efficiency);
			lift_vector = MATHVECTOR <Dbl, 3> (drag, 0, lift);
			
			MATHVECTOR <Dbl, 3> force = drag_vector + lift_vector;
			
			return force;
		}
		
		Dbl GetAerodynamicDownforceCoefficient() const
		{
			return 0.5 * air_density * lift_coefficient * lift_surface_area;
		}
		
		Dbl GetAeordynamicDragCoefficient() const
		{
			return 0.5 * air_density * (drag_coefficient * drag_frontal_area + lift_coefficient * lift_coefficient * lift_surface_area * (1.0-lift_efficiency));
		}
		
		bool Serialize(joeserialize::Serializer & s)
		{
			return true;
		}

		MATHVECTOR< Dbl, 3 > GetLiftVector() const
		{
			return lift_vector;
		}

		MATHVECTOR< Dbl, 3 > GetDragVector() const
		{
			return drag_vector;
		}
};

#endif
