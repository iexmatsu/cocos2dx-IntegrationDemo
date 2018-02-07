#include "SceneBGM.h"
#include "SceneMain.h"
#include "WwiseWrapper.h"
#include "Platform.h"
#include "PlatformCocos.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine
#include "../WwiseProject/GeneratedSoundBanks/Wwise_IDs.h"		// IDs generated by Wwise

#include <string>
#define  LOG_TAG    __FILE__

USING_NS_CC;

#define GAME_OBJECT_RECORDABLE 10
#define GAME_OBJECT_NON_RECORDABLE 20

#if defined AK_PS4 || defined AK_XBOXONE
#define BGM_OUTPUT_TYPE ((AkAudioOutputType)( AkOutput_BGM ))
#define BGM_OUTPUT_FLAGS AkAudioOutputFlags_OptionNotRecordable
#endif

static const AkGameObjectID SECONDARY_LISTENER_ID = 10001;

SceneBGM::SceneBGM()
: SceneBase("Background Music Demo",
    "This demo shows how to setup the background music so the DVR doesn't record it. "
    "This is necessary on platforms that support recording features (DVR) and have a TCR to enforce the proper use of licensed music. "
    "Both streams will be muted when the OS-provided music player starts.",  SceneMain::createScene)
{
    m_bPlayLicensed = m_bPlayCopyright = false;
}

SceneBGM::~SceneBGM()
{
}

Scene* SceneBGM::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    // 'layer' is an autorelease object
    auto layer = SceneBGM::create();
    // add layer as a child to scene
    scene->addChild(layer);
    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool SceneBGM::init()
{
    //////////////////////////////
    // 1. super init first
    if (!SceneBase::init())
    {
	return false;
    }
    cocos2d::Size windowSize = Director::getInstance()->getWinSize();
    float descriptionPosY = windowSize.height * SCREEN_POS_SCALER_DESCRIPTION_Y;
    float selectButtonPosX = g_isLandscape ? windowSize.width * SCREEN_POS_SCALER_SELBUTTON_X : windowSize.height * SCREEN_POS_SCALER_SELBUTTON_X;


    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.
    {

	int y = descriptionPosY;
	// Add button linking to the Say "Hello"
	{
	    auto selectItem = MenuItemImage::create("PlayNormal.png", "PlayPush.png");
	    y -= selectItem->getContentSize().height;
	    addItem(selectItem, selectButtonPosX, y, this);
	    addLabelEx("Play recordable music", selectItem->getPosition().x + selectItem->getContentSize().width, selectItem->getPosition().y, FONT_SIZE_MENU, this, CC_CALLBACK_1(SceneBGM::Recordable_LabelPressed, this));
	}

	// Add button linking to RTPC Demo (Car Engine)
	{
	    auto selectItem = MenuItemImage::create("PlayNormal.png", "PlayPush.png");
	    y -= selectItem->getContentSize().height;
	    addItem(selectItem, selectButtonPosX, y, this);
		addLabelEx("Play non-recordable music", selectItem->getPosition().x + selectItem->getContentSize().width, selectItem->getPosition().y, FONT_SIZE_MENU, this, CC_CALLBACK_1(SceneBGM::NonRecordable_LabelPressed, this));
    }
    }

    AkBankID bankID; // Not used
    if (AK::SoundEngine::LoadBank("BGM.bnk", AK_DEFAULT_POOL_ID, bankID) != AK_Success)
    {
	SetLoadFileErrorMessage("BGM.bnk");
	return false;
    }

	AK::SoundEngine::RegisterGameObj(SECONDARY_LISTENER_ID, "Secondary Listener");


    //Add a secondary output tied to the BGM endpoint of the console.
    //This output will be tied to listener #8 (any can be used, as long as no other output uses it)
#ifdef BGM_OUTPUT_TYPE
    AK::SoundEngine::AddSecondaryOutput(0 /*Ignored for BGM*/, BGM_OUTPUT_TYPE, &SECONDARY_LISTENER_ID, 1, BGM_OUTPUT_FLAGS);
#endif

    // In order to show the difference between a recordable sound and a non-recordable sound, let's set up 2 game objects.
    // Register the "Recordable music object" game object.  
    AK::SoundEngine::RegisterGameObj(GAME_OBJECT_RECORDABLE, "Recordable music");
    // Register the "Non-recordable music object" game object
    AK::SoundEngine::RegisterGameObj(GAME_OBJECT_NON_RECORDABLE, "Non-recordable music");
	//Make the non-recordable object emit sound only to listener SECONDARY_LISTENER_ID.  Nothing to do on the other object as by default everything is output to the main output, and is recordable.
	AK::SoundEngine::SetListeners(GAME_OBJECT_NON_RECORDABLE, &SECONDARY_LISTENER_ID, 1);

    m_bPlayLicensed = false;
    m_bPlayCopyright = false;

    scheduleUpdate();

    return true;
}

void SceneBGM::onRelease()
{
    AK::SoundEngine::StopAll();

    AK::SoundEngine::UnregisterGameObj(GAME_OBJECT_RECORDABLE);
    AK::SoundEngine::UnregisterGameObj(GAME_OBJECT_NON_RECORDABLE);
    AK::SoundEngine::UnloadBank("BGM.bnk", NULL);

#ifdef BGM_OUTPUT_TYPE
	AK::SoundEngine::RemoveSecondaryOutput(0 /*Ignored for BGM*/, BGM_OUTPUT_TYPE);
#endif
    m_lastMenuIx = 9;
}

void SceneBGM::Recordable_LabelPressed(cocos2d::Ref* pSender)
{
    cocos2d::Label * pLabel = (Label*)((MenuItemLabel*)pSender)->getChildren().at(0);

    if (m_bPlayLicensed)
    {
	AK::SoundEngine::StopAll(GAME_OBJECT_RECORDABLE);
	m_bPlayLicensed = false;
	pLabel->setString(std::string("Play recordable music"));
    }
    else
    {
	// Plays the music on the game object linked to the main output.
	AK::SoundEngine::PostEvent("Play_RecordableMusic", GAME_OBJECT_RECORDABLE);
	m_bPlayLicensed = true;
	pLabel->setString(std::string("Stop"));
    }
}

void SceneBGM::NonRecordable_LabelPressed(cocos2d::Ref* pSender)
{
    cocos2d::Label * pLabel = (Label*)((MenuItemLabel*)pSender)->getChildren().at(0);

    if (m_bPlayCopyright)
    {
	AK::SoundEngine::StopAll(GAME_OBJECT_NON_RECORDABLE);
	m_bPlayCopyright = false;
	pLabel->setString("Play non-recordable music");
    }
    else
    {
	// Plays the non-recordable music on the game object linked to the listener that outputs on the BGM end-point.
	AK::SoundEngine::PostEvent("Play_NonRecordableMusic", GAME_OBJECT_NON_RECORDABLE);
	m_bPlayCopyright = true;
	pLabel->setString("Stop");
    }
}
