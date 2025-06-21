﻿/**
* This file is part of Special K.
*
* Special K is free software : you can redistribute it
* and/or modify it under the terms of the GNU General Public License
* as published by The Free Software Foundation, either version 3 of
* the License, or (at your option) any later version.
*
* Special K is distributed in the hope that it will be useful,
*
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Special K.
*
*   If not, see <http://www.gnu.org/licenses/>.
*
**/

#pragma once

#include <SpecialK/thread.h>
#include <SpecialK/utility/lazy_global.h>

#include <render/ngx/ngx_defs.h>

struct NGX_ThreadSafety {
  struct {
    SK_Thread_HybridSpinlock Params;
  } locks;
};

extern SK_LazyGlobal <NGX_ThreadSafety> SK_NGX_Threading;

void SK_NGX_EstablishDLSSVersion  (const wchar_t*) noexcept;
void SK_NGX_EstablishDLSSGVersion (const wchar_t*) noexcept;

struct SK_DLSS_Context
{
  // Has the DLSS context for this API (i.e. D3D11, D3D12, VULKAN)
  //   made any API calls?
  bool apis_called               = false;

  struct version_s {
    unsigned int major           = 0,
                 minor           = 0,
                 build           = 0,
                 revision        = 0;
    bool         driver_override = false;

    bool isOlderThan (version_s& test)
    {
      return
        test.major     > major   || ( test.major    == major   &&
                                      test.minor     > minor ) ||
      ( test.major    == major &&
        test.minor    == minor &&
        test.build     > build ) || ( test.major    == major &&
                                      test.minor    == minor &&
                                      test.build    == build &&
                                      test.revision  > revision );
    }
  };

  struct dlss_s {
    struct instance_s {
      NVSDK_NGX_Handle*    Handle     = nullptr;
      NVSDK_NGX_Parameter* Parameters = nullptr;
      NVSDK_NGX_Feature    DLSS_Type  = NVSDK_NGX_Feature_SuperSampling;
    };
    concurrency::concurrent_unordered_map <const NVSDK_NGX_Handle*, instance_s>
                         Instances {};
    instance_s*          LastInstance   = nullptr;
    volatile ULONG64     LastFrame      = 0ULL;
    volatile ULONG64     ResetFrame     = 0ULL; // If >= Current Frame, issue a DLSS Reset
    static DWORD         IndicatorFlags;
    static version_s     Version;

    static bool hasSharpening       (void) { return ( Version.major <= 2   && ( Version.major != 2   ||   Version.minor <  5 ||   Version.build < 1 ) );                             };
    static bool hasDLAAQualityLevel (void) { return ( Version.major  > 3   || ( Version.major == 3   && ( Version.minor >  1 || ( Version.minor == 1 && Version.build >= 13 ) ) ) ); };
    static bool hasAlphaUpscaling   (void) { return ( Version.major  > 3   || ( Version.major == 3   && ( Version.minor >  6 )                                                  ) ); };
    static bool hasPresetE          (void) { return ( Version.major  > 3   || ( Version.major == 3   && ( Version.minor >  6 )                                                  ) ); };
    static bool hasPresetsAThroughD (void) { return ( Version.major  < 3   ||   
                                                      Version.major >= 310 || ( Version.major == 3   && ( Version.minor <  8 || ( Version.minor == 8 && Version.build <=  9 ) ) ) ); };
    static bool hasPresetsAThroughE (void) { return ( hasPresetsAThroughD()&&
                                                     (Version.major  < 310 || ( Version.minor  < 3 )                                                                            ) ); };
    static bool hasPresetJ          (void) { return ( Version.major  > 310 || ( Version.major == 310 && ( Version.minor >= 1                                                  ) ) ); };
    static bool hasPresetK          (void) { return ( Version.major  > 310 || ( Version.major == 310 && ( Version.minor >  2 || ( Version.minor == 2 && Version.build >=  1 ) ) ) ); };

    static void showIndicator    (bool show);
    static bool isIndicatorShown (void);

    bool        hasInstance (const NVSDK_NGX_Handle* handle) { return Instances.count (handle) != 0; }
    instance_s* getInstance (const NVSDK_NGX_Handle* handle) { if (hasInstance (handle)) return &Instances [handle]; return nullptr; }

    bool evaluateFeature (instance_s* feature)
    {
      if (feature == nullptr)
        return false;

      WriteULong64Release (&LastFrame, SK_GetFramesDrawn ());

      LastInstance = feature;

      return true;
    }
  } super_sampling;

  struct dlssg_s {
    struct instance_s {
      NVSDK_NGX_Handle*    Handle     = nullptr;
      NVSDK_NGX_Parameter* Parameters = nullptr;
    };
    concurrency::concurrent_unordered_map <const NVSDK_NGX_Handle*, instance_s>
                         Instances {};
    instance_s*          LastInstance   = nullptr;
    volatile ULONG64     LastFrame      = 0ULL;
    static DWORD         IndicatorFlags;
    static version_s     Version;

    static void showIndicator    (bool show);
    static bool isIndicatorShown (void);

    bool        hasInstance (const NVSDK_NGX_Handle* handle) { return Instances.count (handle) != 0; }
    instance_s* getInstance (const NVSDK_NGX_Handle* handle) { if (hasInstance (handle)) return &Instances [handle]; return nullptr; }

    bool evaluateFeature (instance_s* feature)
    {
      if (feature == nullptr)
        return false;

      WriteULong64Release (&LastFrame, SK_GetFramesDrawn ());

      LastInstance = feature;

      return true;
    }
  } frame_gen;

  inline void log_call (void) noexcept { apis_called = true; SK_NGX_EstablishDLSSVersion (L"nvngx_dlss.dll"); };
};

extern SK_DLSS_Context SK_NGX_VULKAN;
extern SK_DLSS_Context SK_NGX_DLSS12;
extern SK_DLSS_Context SK_NGX_DLSS11;

void             NVSDK_CONV NVSDK_NGX_Parameter_SetI_Detour           (      NVSDK_NGX_Parameter* InParameter, const char* InName, int                  InValue);
void             NVSDK_CONV NVSDK_NGX_Parameter_SetUI_Detour          (      NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int         InValue);
void             NVSDK_CONV NVSDK_NGX_Parameter_SetULL_Detour         (      NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned long long   InValue);
void             NVSDK_CONV NVSDK_NGX_Parameter_SetF_Detour           (      NVSDK_NGX_Parameter* InParameter, const char* InName, float                InValue);
void             NVSDK_CONV NVSDK_NGX_Parameter_SetD_Detour           (      NVSDK_NGX_Parameter* InParameter, const char* InName, double               InValue);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_Parameter_GetVoidPointer_Detour (const NVSDK_NGX_Parameter* InParameter, const char* InName, void              **OutValue);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_Parameter_GetI_Detour           (const NVSDK_NGX_Parameter* InParameter, const char* InName, int                *OutValue);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_Parameter_GetUI_Detour          (const NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int       *OutValue);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_Parameter_GetULL_Detour         (const NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned long long *OutValue);

using NVSDK_NGX_Parameter_SetF_pfn           = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName,          float      InValue);
using NVSDK_NGX_Parameter_SetD_pfn           = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName,          double     InValue);
using NVSDK_NGX_Parameter_SetI_pfn           = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName,          int        InValue);
using NVSDK_NGX_Parameter_SetUI_pfn          = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int        InValue);
using NVSDK_NGX_Parameter_SetULL_pfn         = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned long long  InValue);
using NVSDK_NGX_Parameter_GetULL_pfn         = NVSDK_NGX_Result (NVSDK_CONV *)(const NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned long long* OutValue);
using NVSDK_NGX_Parameter_GetUI_pfn          = NVSDK_NGX_Result (NVSDK_CONV *)(const NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int*       OutValue);
using NVSDK_NGX_Parameter_GetI_pfn           = NVSDK_NGX_Result (NVSDK_CONV *)(const NVSDK_NGX_Parameter *InParameter, const char* InName,          int*       OutValue);
using NVSDK_NGX_Parameter_GetVoidPointer_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(const NVSDK_NGX_Parameter *InParameter, const char* InName, void**              OutValue);

extern NVSDK_NGX_Parameter_SetF_pfn           NVSDK_NGX_Parameter_SetF_Original;
extern NVSDK_NGX_Parameter_SetD_pfn           NVSDK_NGX_Parameter_SetD_Original;
extern NVSDK_NGX_Parameter_SetI_pfn           NVSDK_NGX_Parameter_SetI_Original;
extern NVSDK_NGX_Parameter_SetUI_pfn          NVSDK_NGX_Parameter_SetUI_Original;
extern NVSDK_NGX_Parameter_SetULL_pfn         NVSDK_NGX_Parameter_SetULL_Original;
extern NVSDK_NGX_Parameter_GetULL_pfn         NVSDK_NGX_Parameter_GetULL_Original;
extern NVSDK_NGX_Parameter_GetUI_pfn          NVSDK_NGX_Parameter_GetUI_Original;
extern NVSDK_NGX_Parameter_GetI_pfn           NVSDK_NGX_Parameter_GetI_Original;
extern NVSDK_NGX_Parameter_GetVoidPointer_pfn NVSDK_NGX_Parameter_GetVoidPointer_Original;

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
using  GetNGXResultAsString_pfn = const wchar_t* (NVSDK_CONV *)(NVSDK_NGX_Result InNGXResult);
extern GetNGXResultAsString_pfn GetNGXResultAsString;

bool SK_NGX_HookParameters (NVSDK_NGX_Parameter *Params);
void SK_NGX_Reset          (void);

void SK_NGX_DumpParameters (const NVSDK_NGX_Parameter *Params);

void
NVSDK_CONV
SK_NGX_LogCallback ( const char*             message,
                     NVSDK_NGX_Logging_Level loggingLevel,
                     NVSDK_NGX_Feature       sourceComponent );

const char*
SK_NGX_FeatureToStr (NVSDK_NGX_Feature feature) noexcept;

extern bool SK_NGX_DLSSG_LateInject;

extern void *SK_NGX_DLSSG_UI_Buffer;
extern void *SK_NGX_DLSSG_HUDLess_Buffer;
extern void *SK_NGX_DLSSG_Back_Buffer;
extern void *SK_NGX_DLSSG_MVecs_Buffer;
extern void *SK_NGX_DLSSG_Depth_Buffer;

void SK_NGX_Init              (void);
void SK_NGX_UpdateDLSSGStatus (void);