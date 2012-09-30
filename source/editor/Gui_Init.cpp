#include "pch.h"
#include "../ogre/common/Defines.h"
#include "OgreApp.h"
#include "../vdrift/pathmanager.h"

#include "../ogre/common/Gui_Def.h"
#include "../ogre/common/MultiList2.h"
#include "../ogre/common/Slider.h"
#include <boost/filesystem.hpp>

#include "../shiny/Main/Factory.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MyGUI;
using namespace Ogre;


///  Gui Init
//----------------------------------------------------------------------------------------------------------------------

void App::InitGui()
{
	if (!mGUI)  return;
	QTimer ti;  ti.update();  /// time

	//  new widgets
	MyGUI::FactoryManager::getInstance().registerFactory<MultiList2>("Widget");
	MyGUI::FactoryManager::getInstance().registerFactory<Slider>("Widget");
	int i;

	//  load layout - wnds
	vwGui = LayoutManager::getInstance().loadLayout("Editor.layout");
	for (VectorWidgetPtr::iterator it = vwGui.begin(); it != vwGui.end(); ++it)
	{
		const std::string& name = (*it)->getName();
		setToolTips((*it)->getEnumerator());
		if (name == "MainMenuWnd"){  mWndMain = *it;	} else
		if (name == "EditorWnd")  {  mWndEdit = *it;	} else
		if (name == "OptionsWnd") {  mWndOpts = *it;	} else
		if (name == "HelpWnd")    {  mWndHelp = *it;	} else

		if (name == "CamWnd")     {  mWndCam = *it;		(*it)->setPosition(0,64);	} else
		if (name == "StartWnd")   {  mWndStart = *it;	(*it)->setPosition(0,64);	} else
		if (name == "BrushWnd")   {  mWndBrush = *it;	(*it)->setPosition(0,64);	} else

		if (name == "RoadCur")    {  mWndRoadCur = *it;		(*it)->setPosition(0,34);	} else
		if (name == "RoadStats")  {  mWndRoadStats = *it;	(*it)->setPosition(0,328);	} else

		if (name == "FluidsWnd")  {  mWndFluids = *it;	(*it)->setPosition(0,64);	} else
		if (name == "ObjectsWnd") {  mWndObjects = *it;	(*it)->setPosition(0,64);	}
	}
	if (mWndRoadStats)  mWndRoadStats->setVisible(false);

	//  main menu
	for (i=0; i < WND_ALL; ++i)
	{
		const String s = toStr(i);
		mWndMainPanels[i] = mWndMain->findWidget("PanMenu"+s);
		mWndMainBtns[i] = (ButtonPtr)mWndMain->findWidget("BtnMenu"+s);
		mWndMainBtns[i]->eventMouseButtonClick += newDelegate(this, &App::MainMenuBtn);
	}
	//  center
	int sx = mWindow->getWidth(), sy = mWindow->getHeight();
	IntSize w = mWndMain->getSize();
	mWndMain->setPosition((sx-w.width)*0.5f, (sy-w.height)*0.5f);


	GuiInitTooltip();
	
	//  assign controls, tool window texts  ----------------------
	if (mWndBrush)
	for (i=0; i<BR_TXT; ++i)
	{
		brTxt[i] = mGUI->findWidget<StaticText>("brTxt"+toStr(i),false);
		brVal[i] = mGUI->findWidget<StaticText>("brVal"+toStr(i),false);
		brKey[i] = mGUI->findWidget<StaticText>("brKey"+toStr(i),false);
	}
	brImg = mGUI->findWidget<StaticImage>("brushImg", false);

	if (mWndRoadCur)
	for (i=0; i<RD_TXT; ++i)
	{	rdTxt[i] = mGUI->findWidget<StaticText>("rdTxt"+toStr(i),false);
		rdVal[i] = mGUI->findWidget<StaticText>("rdVal"+toStr(i),false);
		rdKey[i] = mGUI->findWidget<StaticText>("rdKey"+toStr(i),false);
	}
	if (mWndRoadStats)
	for (i=0; i<RDS_TXT; ++i)
	{	rdTxtSt[i] = mGUI->findWidget<StaticText>("rdTxtSt"+toStr(i),false);
		rdValSt[i] = mGUI->findWidget<StaticText>("rdValSt"+toStr(i),false);
	}
	
	if (mWndStart)
		for (i=0; i<ST_TXT; ++i)	stTxt[i] = mGUI->findWidget<StaticText>("stTxt"+toStr(i),false);

	if (mWndFluids)
		for (i=0; i<FL_TXT; ++i)	flTxt[i] = mGUI->findWidget<StaticText>("flTxt"+toStr(i),false);
		
	if (mWndObjects)
		for (i=0; i<OBJ_TXT; ++i)	objTxt[i] = mGUI->findWidget<StaticText>("objTxt"+toStr(i),false);
	objPan = mGUI->findWidget<Widget>("objPan",false);  if (objPan)  objPan->setVisible(false);
		
	//  Tabs
	TabPtr tab;
	tab = mGUI->findWidget<Tab>("TabWndEdit");  mWndTabsEdit = tab;  tab->setIndexSelected(1);  tab->eventTabChangeSelect += newDelegate(this, &App::MenuTabChg);
	tab = mGUI->findWidget<Tab>("TabWndOpts");	mWndTabsOpts = tab;	 tab->setIndexSelected(1);	tab->eventTabChangeSelect += newDelegate(this, &App::MenuTabChg);
	tab = mGUI->findWidget<Tab>("TabWndHelp");	mWndTabsHelp = tab;	 tab->setIndexSelected(1);	tab->eventTabChangeSelect += newDelegate(this, &App::MenuTabChg);

	//  Options
	if (mWndOpts)
	{	/*mWndOpts->setVisible(false);
		int sx = mWindow->getWidth(), sy = mWindow->getHeight();
		IntSize w = mWndOpts->getSize();  // center
		mWndOpts->setPosition((sx-w.width)*0.5f, (sy-w.height)*0.5f);*/

		//  get sub tabs
		vSubTabsEdit.clear();
		TabPtr sub;
		for (size_t i=0; i < mWndTabsEdit->getItemCount(); ++i)
		{
			sub = (TabPtr)mWndTabsEdit->getItemAt(i)->findWidget("SubTab");
			vSubTabsEdit.push_back(sub);  // 0 for not found
		}
		vSubTabsHelp.clear();
		for (size_t i=0; i < mWndTabsHelp->getItemCount(); ++i)
		{
			sub = (TabPtr)mWndTabsHelp->getItemAt(i)->findWidget("SubTab");
			vSubTabsHelp.push_back(sub);
		}
		vSubTabsOpts.clear();
		for (size_t i=0; i < mWndTabsOpts->getItemCount(); ++i)
		{
			sub = (TabPtr)mWndTabsOpts->getItemAt(i)->findWidget("SubTab");
			vSubTabsOpts.push_back(sub);
		}
		//mWndTabs->setIndexSelected(3);  //default*--
		ResizeOptWnd();
	}

	//  center mouse pos
	PointerManager::getInstance().setVisible(bGuiFocus || !bMoveCam);
	GuiCenterMouse();
	
	//  hide  ---
	SetEdMode(ED_Deform);  UpdEditWnds();  // *  UpdVisHit(); //after track
	if (!mWndOpts) 
	{
		LogO("WARNING: failed to create options window");
		return;  // error
	}
	
	ButtonPtr btn, bchk;  ComboBoxPtr combo;  // for defines
	Slider* sl;

	///  [Graphics]
	//------------------------------------------------------------------------
	GuiInitGraphics();


	///  [Settings]
	//------------------------------------------------------------------------
	Chk("Minimap", chkMinimap, pSet->trackmap);
	Slv(SizeMinmap,	(pSet->size_minimap-0.15f) /1.85f);
	Slv(CamSpeed, powf((pSet->cam_speed-0.1f) / 3.9f, 1.f));
	Slv(CamInert, pSet->cam_inert);
	Slv(TerUpd, pSet->ter_skip /20.f);
	Slv(MiniUpd, pSet->mini_skip /20.f);
	Slv(SizeRoadP, (pSet->road_sphr-0.1f) /11.9f);
	Chk("AutoBlendmap", chkAutoBlendmap, pSet->autoBlendmap);  chAutoBlendmap = bchk;
	Chk("CamPos", chkCamPos, pSet->camPos);
	Chk("InputBar", chkInputBar, pSet->inputBar);  chInputBar = bchk;

	//  set camera btns
	Btn("CamView1", btnSetCam);  Btn("CamView2", btnSetCam);
	Btn("CamView3", btnSetCam);  Btn("CamView4", btnSetCam);
	Btn("CamTop", btnSetCam);
	Btn("CamLeft", btnSetCam);   Btn("CamRight", btnSetCam);
	Btn("CamFront", btnSetCam);  Btn("CamBack", btnSetCam);

	//  startup
	Chk("MouseCapture", chkMouseCapture, pSet->capture_mouse);
	Chk("OgreDialog", chkOgreDialog, pSet->ogre_dialog);
	Chk("AutoStart", chkAutoStart, pSet->autostart);
	Chk("EscQuits", chkEscQuits, pSet->escquit);
	bnQuit = mGUI->findWidget<Button>("Quit");
	if (bnQuit)  {  bnQuit->eventMouseButtonClick += newDelegate(this, &App::btnQuit);  bnQuit->setVisible(false);  }
	

	///  [Sun]
	//----------------------------------------------------------------------------------------------
	Slv(SunPitch,0);  Slv(SunYaw,0);
	Slv(FogStart,0);  Slv(FogEnd,0);
	Chk("FogDisable", chkFogDisable, pSet->bFog);  chkFog = bchk;
	Chk("WeatherDisable", chkWeatherDisable, pSet->bWeather);  chkWeather = bchk;
	Ed(LiAmb, editLiAmb);  Ed(LiDiff, editLiDiff);  Ed(LiSpec, editLiSpec);  Ed(FogClr, editFogClr);
	clrAmb = mGUI->findWidget<ImageBox>("ClrAmb");		clrDiff = mGUI->findWidget<ImageBox>("ClrDiff");
	clrSpec = mGUI->findWidget<ImageBox>("ClrSpec");	clrFog = mGUI->findWidget<ImageBox>("ClrFog");
	clrTrail = mGUI->findWidget<ImageBox>("ClrTrail");
	//Todo: on click event - open color r,g,b dialog
	Slv(Rain1Rate,0);  Slv(Rain2Rate,0);


	///  [Terrain]
	//------------------------------------------------------------------------
	imgTexDiff = mGUI->findWidget<StaticImage>("TerImgDiff");
	Tab(tabsHmap, "TabHMapSize", tabHmap);
	Tab(tabsTerLayers, "TabTerLay", tabTerLayer);

	Edt(edTerTriSize, "edTerTriSize", editTerTriSize);
	Edt(edTerLScale, "edTerLScale", editTerLScale);
	Slv(TerTriSize,	powf((sc.td.fTriangleSize -0.1f)/5.9f, 0.5f));
	Slv(TerLScale, 0);  sldTerLScale = sl;
	Btn("TerrainNew", btnTerrainNew);
	Btn("TerrainGenerate", btnTerGenerate);
	Btn("TerrainHalf", btnTerrainHalf);
	Btn("TerrainDouble", btnTerrainDouble);
	Btn("TerrainMove", btnTerrainMove);

	for (i=0; i < brSetsNum; ++i)  // brush preset
	{
		const String s = toStr(i);  const BrushSet& st = brSets[i];
		StaticImage* img = mGUI->findWidget<StaticImage>("brI"+s,false);
		img->eventMouseButtonClick += newDelegate(this, &App::btnBrushPreset);
		img->setUserString("tip", st.name);  img->setNeedToolTip(true);
		img->eventToolTip += newDelegate(this, &App::notifyToolTip);
		
		StaticText* txt = mGUI->findWidget<StaticText>("brT"+s,false);
		txt->setCaption(fToStr(st.Size,0,2));
	}

	Slv(TerGenScale,powf(pSet->gen_scale   /160.f, 1.f/2.f));  // generate
	Slv(TerGenOfsX, (pSet->gen_ofsx+2.f) /4.f);
	Slv(TerGenOfsY, (pSet->gen_ofsy+2.f) /4.f);
	Slv(TerGenOct,  Real(pSet->gen_oct)	/9.f);
	Slv(TerGenFreq, pSet->gen_freq    /0.7f);
	Slv(TerGenPers, pSet->gen_persist /0.7f);
	Slv(TerGenPow,  powf(pSet->gen_pow     /6.f,  1.f/2.f));


	///  [Layers]
	Chk("TerLayOn", chkTerLayOn, 1);  chkTerLay = bchk;
	valTerLAll = mGUI->findWidget<StaticText>("TerLayersAll");
	Chk("TexNormAuto", chkTexNormAutoOn, 1);  chkTexNormAuto = bchk;
	
	Slv(TerLAngMin,0);  Slv(TerLHMin,0);  Slv(TerLAngSm,0);  // blendmap
	Slv(TerLAngMax,0);  Slv(TerLHMax,0);  Slv(TerLHSm,0);
	Slv(TerLNoise,0);   Chk("TerLNoiseOnly", chkTerLNoiseOnlyOn, 0);  chkTerLNoiseOnly = bchk;
	
	Ed(LDust, editLDust);	Ed(LDustS, editLDust);
	Ed(LMud,  editLDust);	Ed(LSmoke, editLDust);
	Ed(LTrlClr, editLTrlClr);
	Cmb(cmbParDust, "CmbParDust", comboParDust);
	Cmb(cmbParMud,  "CmbParMud",  comboParDust);
	Cmb(cmbParSmoke,"CmbParSmoke",comboParDust);

	Cmb(cmbSurfType, "SurfType", comboSurfType);
	Ed(SuBumpWave, editSurf);	Ed(SuBumpAmp, editSurf);
	Ed(SuRollDrag, editSurf);
	Ed(SuFrict, editSurf);	Ed(SuFrict2, editSurf);

	
	///  [Vegetation]  ------------------------------------
	Ed(GrassDens, editTrGr);  Ed(TreesDens, editTrGr);
	Ed(GrPage, editTrGr);  Ed(GrDist, editTrGr);  Ed(TrPage, editTrGr);  Ed(TrDist, editTrGr);
	Ed(GrMinX, editTrGr);  Ed(GrMaxX, editTrGr);  Ed(GrMinY, editTrGr);  Ed(GrMaxY, editTrGr);
	Ed(GrSwayDistr, editTrGr);  Ed(GrSwayLen, editTrGr);  Ed(GrSwaySpd, editTrGr);
	Ed(TrRdDist, editTrGr);  Ed(TrImpDist, editTrGr);
	Ed(GrDensSmooth, editTrGr);  Ed(SceneryId, editTrGr);
	Ed(GrTerMaxAngle, editTrGr);  Ed(GrTerMinHeight, editTrGr);  Ed(GrTerMaxHeight, editTrGr);
	Cmb(cmbGrassMtr, "CmbGrMtr", comboGrassMtr);
	Cmb(cmbGrassClr, "CmbGrClr", comboGrassClr);

	imgPaged = mGUI->findWidget<StaticImage>("ImgPaged");
	Chk("LTrEnabled", chkPgLayOn, 1);  chkPgLay = bchk;
	valLTrAll = mGUI->findWidget<StaticText>("LTrAll");
	Tab(tabsPgLayers, "LTrNumTab", tabPgLayers);
	Slv(LTrDens, 0);	Slv(LTrRdDist, 0);
	Slv(LTrMinSc, 0);	Slv(LTrMaxSc, 0);
	Slv(LTrWindFx, 0);	Slv(LTrWindFy, 0);
	Slv(LTrMaxTerAng, 0);  Ed(LTrMinTerH, editLTrMinTerH);  Ed(LTrMaxTerH, editLTrMaxTerH);
	Ed(LTrFlDepth, editLTrFlDepth);

	
	///  [Road]  ------------------------------------
	Ed(RdTcMul, editRoad);  Ed(RdLenDim, editRoad);  Ed(RdWidthSteps,editRoad);
	Ed(RdHeightOfs, editRoad);  Ed(RdSkirtLen, editRoad);  Ed(RdSkirtH, editRoad);
	Ed(RdMergeLen, editRoad);  Ed(RdLodPLen, editRoad);
	Ed(RdColN, editRoad);  Ed(RdColR, editRoad);
	Ed(RdPwsM, editRoad);  Ed(RdPlsM, editRoad);
	

	///  [Tools]  ------------------------------------
	Btn("TrackCopySel", btnTrkCopySel);
	valTrkCpySel = mGUI->findWidget<StaticText>("TrkCopySelName");
	Btn("CopySun", btnCopySun);				Btn("CopyTerHmap", btnCopyTerHmap);
	Btn("CopyTerLayers", btnCopyTerLayers);	Btn("CopyVeget", btnCopyVeget);
	Btn("CopyRoad", btnCopyRoad);			Btn("CopyRoadPars", btnCopyRoadPars);
	Btn("DeleteRoad", btnDeleteRoad);		Btn("DeleteFluids", btnDeleteFluids);
	Btn("ScaleAll", btnScaleAll);	Ed(ScaleAllMul, editScaleAllMul);
	Btn("ScaleTerH", btnScaleTerH);	Ed(ScaleTerHMul, editScaleTerHMul);

	Slv(AlignWidthAdd, pSet->al_w_add /20.f);
	Slv(AlignWidthMul, (pSet->al_w_mul-1.f) /4.f);
	Slv(AlignSmooth, pSet->al_smooth /6.f);
	

	///  Fill Combo boxes  . . . . . . .
	//------------------------------------------------------------------------------------------------------------

	GuiInitLang();
	
	//---------------------  Skies  ---------------------
	Cmb(cmbSky, "SkyCombo", comboSky);

	GetMaterialsFromDef("skies.matdef");
	for (size_t i=0; i < vsMaterials.size(); ++i)
	{	const String& s = vsMaterials[i];
		if (s != "")  cmbSky->addItem(s);  //LogO(s);
	}
	//---------------------  Weather  ---------------------
	Cmb(cmbRain1, "Rain1Cmb", comboRain1);  cmbRain1->addItem("");
	Cmb(cmbRain2, "Rain2Cmb", comboRain2);  cmbRain2->addItem("");

	GetMaterials("weather.particle", true, "particle_system");
	for (size_t i=0; i < vsMaterials.size(); ++i)
	{	const String& s = vsMaterials[i];
		cmbRain1->addItem(s);  cmbRain2->addItem(s);
	}	


	//---------------------  Terrain  ---------------------
	Cmb(cmbTexDiff, "TexDiffuse", comboTexDiff);
	Cmb(cmbTexNorm, "TexNormal", comboTexNorm);  cmbTexNorm->addItem("flat_n.png");

	strlist li;
	GetFolderIndex(PATHMANAGER::GetDataPath() + "/terrain", li);
	GetFolderIndex(PATHMANAGER::GetDataPath() + "/terrain2", li);

	for (strlist::iterator i = li.begin(); i != li.end(); ++i)
	if (!StringUtil::match(*i, "*.txt", false))
	{
		if (!StringUtil::match(*i, "*_prv.*", false))
		if (StringUtil::match(*i, "*_nh.*", false))
			cmbTexNorm->addItem(*i);
		else
			cmbTexDiff->addItem(*i);
	}
	
	//  particles
	GetMaterials("tires.particle", true, "particle_system");
	for (size_t i=0; i < vsMaterials.size(); ++i)
	{	const String& s = vsMaterials[i];
		cmbParDust->addItem(s);  cmbParMud->addItem(s);  cmbParSmoke->addItem(s);
	}
	
	//  surfaces
	for (i=0; i < TRACKSURFACE::NumTypes; ++i)
		cmbSurfType->addItem(csTRKsurf[i]);


	//---------------------  Grass  ---------------------
	GetMaterialsFromDef("grass.matdef");
	for (size_t i=0; i < vsMaterials.size(); ++i)
	{	String s = vsMaterials[i];
		if (s.length() > 5)  //!= "grass")
			cmbGrassMtr->addItem(s);
	}
	GetFolderIndex(PATHMANAGER::GetDataPath() + "/materials", li);
	for (strlist::iterator i = li.begin(); i != li.end(); ++i)
	{
		if (StringUtil::startsWith(*i, "grClr", false))
			cmbGrassClr->addItem(*i);
	}

	//---------------------  Trees  ---------------------
	Cmb(cmbPgLay, "LTrCombo", comboPgLay);
	strlist lt;
	GetFolderIndex(PATHMANAGER::GetDataPath() + "/trees", lt);
	for (strlist::iterator i = lt.begin(); i != lt.end(); ++i)
		if (StringUtil::endsWith(*i,".mesh"))  cmbPgLay->addItem(*i);


	//---------------------  Roads  ---------------------
	GetMaterialsFromDef("road.matdef");
	GetMaterialsFromDef("road_pipe.matdef", false);
	for (size_t i=0; i<4; ++i)
	{
		Cmb(cmbRoadMtr[i], "RdMtr"+toStr(i+1), comboRoadMtr);
		Cmb(cmbPipeMtr[i], "RdMtrP"+toStr(i+1), comboPipeMtr);
		if (i>0)  {  cmbRoadMtr[i]->addItem("");  cmbPipeMtr[i]->addItem("");  }
	}
	for (size_t i=0; i < vsMaterials.size(); ++i)
	{	String s = vsMaterials[i];
		if (StringUtil::startsWith(s,"road") && !StringUtil::startsWith(s,"road_") && !StringUtil::endsWith(s,"_ter"))
			for (int i=0; i<4; ++i)  cmbRoadMtr[i]->addItem(s);
		if (StringUtil::startsWith(s,"pipe") && !StringUtil::startsWith(s,"pipe_"))
			for (int i=0; i<4; ++i)  cmbPipeMtr[i]->addItem(s);
	}

	//---------------------  Objects  ---------------------
	strlist lo;  vObjNames.clear();
	GetFolderIndex(PATHMANAGER::GetDataPath() + "/objects", lo);
	for (strlist::iterator i = lo.begin(); i != lo.end(); ++i)
		if (StringUtil::endsWith(*i,".mesh") && (*i) != "sphere.mesh")
			vObjNames.push_back((*i).substr(0,(*i).length()-5));  //no .ext
	
	objListSt = mGUI->findWidget<List>("ObjListSt");
	objListDyn = mGUI->findWidget<List>("ObjListDyn");
	if (objListSt && objListDyn)
	{
		for (int i=0; i < vObjNames.size(); ++i)
		{	const std::string& name = vObjNames[i];
			if (name != "sphere")
			{
				if (StringUtil::startsWith(name,"pers_",false))
					objListSt->addItem("#E0E070"+name);
				else
				if (boost::filesystem::exists(PATHMANAGER::GetDataPath()+"/objects/"+ name + ".bullet"))
					objListDyn->addItem("#80D0FF"+name);  // dynamic
				else
					objListSt->addItem("#C8C8C8"+name);
		}	}
		//objList->setIndexSelected(0);  //objList->findItemIndexWith(modeSel)
		objListSt->eventListChangePosition += newDelegate(this, &App::listObjsChngSt);
		objListDyn->eventListChangePosition += newDelegate(this, &App::listObjsChngDyn);
	}
	//-----------------------------------------------------

	InitGuiScrenRes();
	

	///  [Track]
	//------------------------------------------------------------------------
	sListTrack = pSet->gui.track;  //! set last
	bListTrackU = pSet->gui.track_user;
	sCopyTrack = "";  //! none
	bCopyTrackU = 0;
	
	//  text desc
	Edt(trkDesc[0], "TrackDesc", editTrkDesc);
	trkName = mGUI->findWidget<Edit>("TrackName");
	if (trkName)  trkName->setCaption(pSet->gui.track);

	GuiInitTrack();
	
	//  btn change,  new, rename, delete
	//Btn("ChangeTrack",	btnChgTrack);
	Btn("TrackNew",		btnTrackNew);
	Btn("TrackRename",	btnTrackRename);
	Btn("TrackDelete",	btnTrackDel);
	
    //  load = new game
    for (int i=1; i<=2; ++i)
    {	Btn("NewGame"+toStr(i), btnNewGame);  }

	CreateGUITweakMtr();

	bGI = true;  // gui inited, gui events can now save vals

	ti.update();  /// time
	float dt = ti.dt * 1000.f;
	LogO(String("::: Time Init Gui: ") + toStr(dt) + " ms");
}


void App::CreateRoadSelMtrs()
{
	GetMaterialsFromDef("road.matdef");
	GetMaterialsFromDef("road_pipe.matdef", false);
	for (size_t i=0; i < vsMaterials.size(); ++i)
		createRoadSelMtr(vsMaterials[i]);
}



///  gui tweak page, material properties
//------------------------------------------------------------------------------------------------------------
const std::string sMtr = "Water_blue";  //combo, changable..

void App::CreateGUITweakMtr()
{
	ScrollView* view = mGUI->findWidget<ScrollView>("TweakView",false);
	if (!view)  return;
	int y = 36;

	#define setOrigPos(widget) \
		widget->setUserString("origPosX", toStr(widget->getPosition().left)); \
		widget->setUserString("origPosY", toStr(widget->getPosition().top)); \
		widget->setUserString("origSizeX", toStr(widget->getSize().width)); \
		widget->setUserString("origSizeY", toStr(widget->getSize().height)); \
		widget->setUserString("RelativeTo", "OptionsWnd");

	sh::MaterialInstance* mat = mFactory->getMaterialInstance(sMtr);
	
	const sh::PropertyMap& props = mat->listProperties();
	for (sh::PropertyMap::const_iterator it = props.begin(); it != props.end(); ++it)
	{
		sh::PropertyValuePtr pv = (*it).second;
		std::string name = (*it).first;
		
		//  get type
		std::string sVal = pv->_getStringValue();
		int size = -1;
		std::vector<std::string> tokens;
		boost::split(tokens, sVal, boost::is_any_of(" "));
		size = tokens.size();

		//LogO("PROP: " + name + "  val: " + sVal + "  type:" + toStr(type));
		const static char ch[6] = "rgbau";
		const static Colour clrsType[5] = {Colour(0.9,0.9,0.7),Colour(0.8,1.0,0.8),
					Colour(0.7,0.85,1.0),Colour(0.7,1.0,1.0),Colour(1.0,1.0,1.0)};

		//  for each component (xy,rgb..)
		for (int i=0; i < size; ++i)
		{
			String nameSi = name + ":" + toStr(size) + "." + toStr(i);  // size and id in name
			float val = boost::lexical_cast<float> (tokens[i]);
			int t = std::min(4,i);  const Colour& clr = clrsType[std::max(0,std::min(4,size-1))];

			//  name text
			int x = 0, xs = 140;
			TextBox* txt = view->createWidget<TextBox>("TextBox", x,y, xs,20, Align::Default, nameSi + ".txt");
			setOrigPos(txt);  txt->setTextColour(clr);
			txt->setCaption(size == 1 ? name : name + "." + ch[t]);

			//  val text
			x += xs;  xs = 60;
			TextBox* txtval = view->createWidget<TextBox>("TextBox", x,y, xs,20, Align::Default, nameSi + ".Val");
			setOrigPos(txtval);  txtval->setTextColour(clr);
			txtval->setCaption(fToStr(val,3,6));
			
			//  slider
			x += xs;  xs = 400;
			Slider* sl = view->createWidget<Slider>("Slider", x,y-1, xs,19, Align::Default, nameSi);
			setOrigPos(sl);  sl->setColour(clr);
			sl->setValue(val);
			if (sl->eventValueChanged.empty())  sl->eventValueChanged += newDelegate(this, &App::slTweak);

			y += 22;
		}
		y += 8;
		
		view->setCanvasSize(1100, y+500);  //?..
		view->setCanvasAlign(Align::Default);
	}
}

///  change material property (float component)
void App::slTweak(Slider* wp, float val)
{
	std::string name = wp->getName(), prop = name.substr(0,name.length()-4);
	TextBox* txtval = mGUI->findWidget<TextBox>(name + ".Val");
	if (txtval)
		txtval->setCaption(fToStr(val,3,6));

	int id = -1, size = 1;
	if (name.substr(name.length()-2,1) == ".")  // more than 1 float
	{
		id = s2i(name.substr(name.length()-1,1));
		size = s2i(name.substr(name.length()-3,1));
	}

	if (id == -1)  // 1 float
		mFactory->getMaterialInstance(sMtr)->setProperty(name, sh::makeProperty<sh::FloatValue>(new sh::FloatValue(val)));
	else
	{
		sh::MaterialInstance* mat = mFactory->getMaterialInstance(sMtr);
		sh::PropertyValuePtr& vp = mat->getProperty(prop);
		switch (size)
		{
			case 2:
			{	sh::Vector2 v = sh::retrieveValue<sh::Vector2>(vp,0);
				((float*)&v.mX)[id] = val;
				mat->setProperty(prop, sh::makeProperty<sh::Vector2>(new sh::Vector2(v.mX, v.mY)));
			}	break;
			case 3:
			{	sh::Vector3 v = sh::retrieveValue<sh::Vector3>(vp,0);
				((float*)&v.mX)[id] = val;
				mat->setProperty(prop, sh::makeProperty<sh::Vector3>(new sh::Vector3(v.mX, v.mY, v.mZ)));
			}	break;
			case 4:
			{	sh::Vector4 v = sh::retrieveValue<sh::Vector4>(vp,0);
				((float*)&v.mX)[id] = val;
				mat->setProperty(prop, sh::makeProperty<sh::Vector4>(new sh::Vector4(v.mX, v.mY, v.mZ, v.mW)));
			}	break;
		}
	}
}
