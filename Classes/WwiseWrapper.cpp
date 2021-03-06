#include <wchar.h>

#include "cocos2d.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/AllPluginsFactories.h>

#ifndef AK_OPTIMIZED
  #include <AK/Comm/AkCommunication.h> // Communication between Wwise and the game (excluded in release build)
#endif // AK_OPTIMIZED
#ifdef AK_MOTION
  #include <AK/MotionEngine/Common/AkMotionEngine.h>	// Motion Engine (required only for playback of Motion objects)
#endif // AK_MOTION

#include "Platform.h"
#include "PlatformCocos.h"

#include "WwiseWrapper.h"
#include "WwisePlatformHelper.h"


Wwise& Wwise::Instance()
{
    static Wwise wwise;
    return wwise;
}

Wwise::Wwise() 
{
    m_pLowLevelIO = new CAkFilePackageLowLevelIOBlocking();
}

Wwise::~Wwise()
{
    delete m_pLowLevelIO;
    m_pLowLevelIO = NULL;
}

CAkFilePackageLowLevelIOBlocking& Wwise::IOManager()
{
    return *m_pLowLevelIO;
}

bool Wwise::Init()
{
    AkMemSettings       memSettings;
    AkStreamMgrSettings     stmSettings;
    AkDeviceSettings        deviceSettings;
    AkInitSettings      initSettings;
    AkPlatformInitSettings  platformInitSettings;
    AkMusicSettings     musicInit;
    AkOSChar            strError[1024];

    GetDefaultSettings(memSettings, stmSettings, deviceSettings, initSettings, platformInitSettings, musicInit);

    if (!ConfigurePlatform(platformInitSettings))
    {
        return false;
    }

    bool bSuccess = false;
    
    bSuccess = InitWwise(memSettings, stmSettings, deviceSettings, initSettings, platformInitSettings, musicInit, strError, sizeof(strError));
    if (!bSuccess) {
        LOGAKW(strError);
        goto cleanup;
    }

    if ( ! ConfigureSoundBankPaths() )
    {
         goto cleanup;
    }

    // Set global language. Low-level I/O devices can use this string to find language-specific assets.
    if (AK::StreamMgr::SetCurrentLanguage(AKTEXT("English(US)")) != AK_Success)
    {
        goto cleanup;
    }

    AkBankID bankID;
    if (AK::SoundEngine::LoadBank("Init.bnk", AK_DEFAULT_POOL_ID, bankID) != AK_Success)
    {
        LOGAK("<Wwise::Init> Cannot load Init.bnk! error");
        cocos2d::MessageBox("Cannot load Init.bnk!", "Error");
        goto cleanup;
    }

    return true;

cleanup:
    Term();
    return false;
}

void Wwise::Term()
{
    TermWwise();
}

void Wwise::Update()
{
    AK::SoundEngine::RenderAudio();
}


//
// Private Methods
//

void Wwise::GetDefaultSettings( AkMemSettings&          out_memSettings,
                AkStreamMgrSettings&    out_stmSettings,
                AkDeviceSettings&       out_deviceSettings,
                AkInitSettings&         out_initSettings,
                AkPlatformInitSettings& out_platformInitSettings,
                AkMusicSettings&        out_musicInit)
{
    out_memSettings.uMaxNumPools = 20;
    AK::StreamMgr::GetDefaultSettings(out_stmSettings);

    AK::StreamMgr::GetDefaultDeviceSettings(out_deviceSettings);

    AK::SoundEngine::GetDefaultInitSettings(out_initSettings);
    out_initSettings.uDefaultPoolSize = 2 * 1024 * 1024;

    AK::SoundEngine::GetDefaultPlatformInitSettings(out_platformInitSettings);
    out_platformInitSettings.uLEngineDefaultPoolSize = 1 * 1024 * 1024;

    AK::MusicEngine::GetDefaultInitSettings(out_musicInit);
}


bool Wwise::InitWwise(	AkMemSettings&          in_memSettings,
			AkStreamMgrSettings&    in_stmSettings,
			AkDeviceSettings&       in_deviceSettings,
			AkInitSettings&         in_initSettings,
			AkPlatformInitSettings& in_platformInitSettings,
			AkMusicSettings&        in_musicInit,
			AkOSChar*               in_szErrorBuffer,
			unsigned int            in_unErrorBufferCharCount)
{
    //
    // Create and initialize an instance of the default memory manager. Note
    // that you can override the default memory manager with your own. Refer
    // to the SDK documentation for more information.
    //

    AKRESULT res = AK::MemoryMgr::Init(&in_memSettings);
    if (res != AK_Success)
    {
    	__AK_OSCHAR_SNPRINTF(in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::MemoryMgr::Init() returned AKRESULT %d"), res);
    	LOGAKW(in_szErrorBuffer);
    	return false;
    }

    //
    // Create and initialize an instance of the default streaming manager. Note
    // that you can override the default streaming manager with your own. Refer
    // to the SDK documentation for more information.
    //

    // Customize the Stream Manager settings here.

    if (!AK::StreamMgr::Create(in_stmSettings))
    {
    	AKPLATFORM::SafeStrCpy(in_szErrorBuffer, AKTEXT("AK::StreamMgr::Create() failed"), in_unErrorBufferCharCount);
    	LOGAKW(in_szErrorBuffer);
    	return false;
    }

    //
    // Create a streaming device with blocking low-level I/O handshaking.
    // Note that you can override the default low-level I/O module with your own. Refer
    // to the SDK documentation for more information.
    //

    // CAkFilePackageLowLevelIOBlocking::Init() creates a streaming device
    // in the Stream Manager, and registers itself as the File Location Resolver.
    res = m_pLowLevelIO->Init(in_deviceSettings);
    if (res != AK_Success)
    {
    	__AK_OSCHAR_SNPRINTF(in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("m_lowLevelIO.Init() returned AKRESULT %d"), res);
    	LOGAKW(in_szErrorBuffer);
    	return false;
    }

    //
    // Create the Sound Engine
    // Using default initialization parameters
    //

    res = AK::SoundEngine::Init(&in_initSettings, &in_platformInitSettings);
    if (res != AK_Success)
    {
    	__AK_OSCHAR_SNPRINTF(in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::SoundEngine::Init() returned AKRESULT %d"), res);
    	LOGAKW(in_szErrorBuffer);
    	return false;
    }

    //
    // Initialize the music engine
    // Using default initialization parameters
    //

    res = AK::MusicEngine::Init(&in_musicInit);
    if (res != AK_Success)
    {
    	__AK_OSCHAR_SNPRINTF(in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::MusicEngine::Init() returned AKRESULT %d"), res);
    	LOGAKW(in_szErrorBuffer);
    	return false;
    }

#if defined(NDK_DEBUG) || defined(AK_DEBUG) || defined(_DEBUG)
    //
    // Initialize communications (not in release build!)
    //
	AkCommSettings commSettings;
    AK::Comm::GetDefaultInitSettings( commSettings );
    AKPLATFORM::SafeStrCpy(commSettings.szAppNetworkName, "Cocos2d-x Integration Demo", AK_COMM_SETTINGS_MAX_STRING_SIZE);
    res = AK::Comm::Init( commSettings );
	if (res != AK_Success)
	{
	    __AK_OSCHAR_SNPRINTF(in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible."), res);
	    LOGAKW(in_szErrorBuffer);
	}
#endif

    AK::SoundEngine::RegisterGameObj(LISTENER_ID, "Listener (Default)");
    AK::SoundEngine::SetDefaultListeners(&LISTENER_ID, 1);
    
    return true;
}

void Wwise::TermWwise()
{
//#ifndef AK_OPTIMIZED
#if defined(NDK_DEBUG) || defined(AK_DEBUG) || defined(_DEBUG)
    // Terminate communications between Wwise and the game
	AK::Comm::Term();
#endif
    // Terminate the music engine
    AK::MusicEngine::Term();

    // Terminate the sound engine
    if (AK::SoundEngine::IsInitialized())
    {
	   AK::SoundEngine::Term();
    }

    // Terminate the streaming device and streaming manager
    // CAkFilePackageLowLevelIOBlocking::Term() destroys its associated streaming device 
    // that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
    if (AK::IAkStreamMgr::Get())
    {
    	m_pLowLevelIO->Term();
    	AK::IAkStreamMgr::Get()->Destroy();
    }

    // Terminate the Memory Manager
    if (AK::MemoryMgr::IsInitialized())
    {
	   AK::MemoryMgr::Term();
    }
}

bool Wwise::ConfigurePlatform(AkPlatformInitSettings& platformInitSettings)
{
    return WwisePlatformHelper::Instance().ConfigurePlatform(platformInitSettings, m_pLowLevelIO);
}

bool Wwise::ConfigureSoundBankPaths()
{
    return WwisePlatformHelper::Instance().ConfigureSoundBankPaths(m_pLowLevelIO);
}
