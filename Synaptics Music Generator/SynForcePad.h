//===========================================================================
// Copyright (c) 1996-2012 Synaptics Incorporated. All rights reserved.
//
// SynCom API demo for ForcePad
//
// SynForcePad.h : main header file for the SynForcePad application
//
// Jan, 2012
//
//===========================================================================

#pragma once

#ifndef __AFXWIN_H__
  #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"   // main symbols

// CSynForcePadApp:
// See SynForcePad.cpp for the implementation of this class
//

class CSynForcePadApp : public CWinApp
{
public:
  CSynForcePadApp();

// Overrides
public:
  virtual BOOL InitInstance();

// Implementation

  DECLARE_MESSAGE_MAP()
};

extern CSynForcePadApp theApp;

