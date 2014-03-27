//===========================================================================
// Copyright (c) 1996-2012 Synaptics Incorporated. All rights reserved.
//
// SynCom API demo for ForcePad
//
// SynForcePadDlg.h : header file
//
// Jan, 2012
//
//===========================================================================

#ifndef __SYNFORCEPADDLG_H_
#define __SYNFORCEPADDLG_H_

#include "SynKit.h"

#define MAX_NUM_FORCESENSORS  4

// CSynForcePadDlg dialog
class CSynForcePadDlg : public CDialog, public _ISynAPIEvents, public _ISynDeviceEvents
{
// Construction
public:
  CSynForcePadDlg(CWnd* pParent = NULL);  // standard constructor

  BOOL Connect();
  VOID Disconnect();

// Dialog Data
  enum { IDD = IDD_SYNFORCEPAD_DIALOG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

// Implementation
protected:
  HICON m_hIcon;

  // Generated message map functions
  virtual BOOL OnInitDialog();
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();

  SynAPI* m_pAPI;
  SynDevice* m_pDevice;

  BOOL  m_bConnected;
  LONG  m_lDeviceHandle;
  BOOL  m_bCanProcessGroups;
  ULONG m_ulMaxGroupSize;
  ULONG m_ulNumForceSensors;

  HRESULT STDMETHODCALLTYPE OnSynDevicePacket(LONG lSeq);
  HRESULT STDMETHODCALLTYPE OnSynAPINotify(LONG lReason);

  DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnClose();
};
#endif

