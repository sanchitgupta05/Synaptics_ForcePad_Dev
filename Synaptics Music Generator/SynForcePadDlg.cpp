
#include <iostream>
#include <stdio.h>
#include "stdafx.h"
#include "SynForcePad.h"
#include "SynForcePadDlg.h"
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "Winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

// CSynForcePadDlg dialog

CSynForcePadDlg::CSynForcePadDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CSynForcePadDlg::IDD, pParent)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  m_bConnected = FALSE;
  m_pAPI = NULL;
  m_pDevice = NULL;
  m_lDeviceHandle = -1;
}

void CSynForcePadDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSynForcePadDlg, CDialog)
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  //}}AFX_MSG_MAP
  ON_WM_CLOSE()
END_MESSAGE_MAP()

// CSynForcePadDlg message handlers

BOOL CSynForcePadDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  // when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);     // Set big icon
  SetIcon(m_hIcon, FALSE);    // Set small icon

  m_pAPI = new SynAPI;
  m_pDevice = new SynDevice;

  if (m_pAPI && m_pDevice) {
    // Add retry procedures in case one of driver components has not loaded yet
    // Connecting... if failed, repeat... until timeout
    for (int i = 0; i < 1000; i++) {
      if (Connect()) {
        break;
      }
      Sleep(100);
    }
  }
  return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.
void CSynForcePadDlg::OnPaint()
{
  if (IsIconic()) {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  } else {
    CDialog::OnPaint();
  }
}

// The system calls this function to obtain the cursor to display while the user drags
// the minimized window.
HCURSOR CSynForcePadDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

// Register to SynCom
BOOL CSynForcePadDlg::Connect()
{
  if (!m_bConnected) {
    // Init...
    if ((*m_pAPI)->Initialize() == SYN_OK &&  // Initialize API
      // Find touchpad in USB port and get its device handle
      (*m_pAPI)->FindDevice(SE_ConnectionUSB, SE_DeviceTouchPad, &m_lDeviceHandle) == SYN_OK &&
      // Select device 
      (*m_pDevice)->Select(m_lDeviceHandle) == SYN_OK)
    {
      // Set event synchronization
      (*m_pAPI)->SetSynchronousNotification(this);
      (*m_pDevice)->SetSynchronousNotification(this);

      // Enable multifinger packets report
      (*m_pDevice)->SetProperty(SP_IsMultiFingerReportEnabled, 1);

      LONG lValue = 0;
      (*m_pDevice)->GetProperty(SP_HasMultiFingerPacketsGrouped, &lValue);
      m_bCanProcessGroups = !!lValue;
      (*m_pDevice)->GetProperty(SP_HasPacketGroupProcessing, &lValue);
      m_bCanProcessGroups = m_bCanProcessGroups && !!lValue;
      // Enable packet group processing (to include force)
      if (m_bCanProcessGroups) {
        (*m_pDevice)->SetProperty(SP_IsGroupReportEnabled, 1);
      }

      (*m_pDevice)->GetProperty(SP_NumMaxReportedFingers, &lValue);
      m_ulMaxGroupSize = (ULONG)lValue;
      (*m_pDevice)->GetProperty(SP_NumForceSensors, &lValue);
      m_ulNumForceSensors = (ULONG)lValue;
      ASSERT(m_ulNumForceSensors <= MAX_NUM_FORCESENSORS);

      m_bConnected = TRUE;
    } else {
      m_lDeviceHandle = -1;
      return FALSE;
    }
  }

  return TRUE;
}

// Unregister to SynCom
VOID CSynForcePadDlg::Disconnect()
{
  if (m_bConnected && m_lDeviceHandle != -1) {
    // Clear synchronization notifications
    (*m_pDevice)->SetSynchronousNotification(0);
    (*m_pAPI)->SetSynchronousNotification(0);
    m_bConnected = FALSE;
    m_lDeviceHandle = -1;
  }
}

// When device configuration is changed by adding/deleting device and power state changes
// device driver will send events notification to the application, which should be handled
// in the function OnSynAPINotify()
HRESULT CSynForcePadDlg::OnSynAPINotify(LONG lReason)
{
  switch (lReason) {
    case SE_DeviceRemoved:
      break;
    case SE_DeviceAdded:
      break;
    case SE_Configuration_Changed:
      Disconnect();
      // Reconnect to the API and the device due to possible configuration changes
      for (int i = 1; i < 1000; i ++) {
        if (Connect()) {
          return 0;
        }
        Sleep(100);
      }
      break;
    default:
      break;
  }
  return -1;
}

// Handle the packet notification
HRESULT CSynForcePadDlg::OnSynDevicePacket(LONG lSeq)
{
  if (m_bCanProcessGroups) {
    SynGroup grp;
    // display raw data
    if (SYN_OK == (*m_pDevice)->LoadGroup(grp)) {
      LONG lGroup = grp.GroupNumber();
      
      LONG lForceGrams[MAX_NUM_FORCESENSORS];
      LONG lForceRaw[MAX_NUM_FORCESENSORS];
      LONG lTotal = 0;
      for(ULONG ul = 0; ul < MAX_NUM_FORCESENSORS; ul++) {
        lForceGrams[ul] = grp.Force(ul);
        lForceRaw[ul] = grp.ForceRaw(ul);
        lTotal += lForceGrams[ul];
      }

      TCHAR tszBuffer[100] = {0};
      // only update every 10th packet group, for readability
      if (!(lGroup % 10)) {
        wsprintf(tszBuffer, TEXT("Force %d %d %d %d  grams: %d %d %d %d total(g): %d\n"),
          lForceRaw[0], lForceRaw[1], lForceRaw[2], lForceRaw[3],
          lForceGrams[0], lForceGrams[1], lForceGrams[2], lForceGrams[3], lTotal );
        SetDlgItemText(IDC_STATIC0, tszBuffer);
      }
	  
      for (ULONG ul = 0; ul < m_ulMaxGroupSize; ul++) {
        SynPacket pkt;
        LONG lX, lY, lZ, lForce, lGroup;
        
        grp.Packet(ul, pkt);
		
		int width = (5888-1024)/8;
        lX = pkt.GetLongProperty(SP_XRaw);				// These ARE ALL THE X, Y, and Z values ....
			printf("%d", lX);							// sending the X vaule
        lY = pkt.GetLongProperty(SP_YRaw);
			printf(" %d", lY);
        lZ = pkt.GetLongProperty(SP_ZRaw);
        lForce = pkt.GetLongProperty(SP_ZForce);
			printf(" %d\n", lForce);
        lGroup = pkt.GetLongProperty(SP_PacketGroupNumber);

        TCHAR tszBuffer[100] = {0};
        // only update every 10th packet group, for readability
        if (!(lGroup % 10)) {
          if (lZ) {
            wsprintf(tszBuffer, TEXT("Finger %d, XRaw %d, YRaw %d, ZRaw %d, Force %d\n"),
              ul, lX, lY, lZ, lForce);
			
			if(lX < 1024)
				PlaySound(NULL, NULL, SND_FILENAME | SND_SYNC);
			if((lX-1024)/width == 0){
				PlaySound(L"a.wav",NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);			
			}
			else if((lX-1024)/width == 1)
			{
				PlaySound(L"b.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
			else if((lX-1024)/width == 2)
			{
				PlaySound(L"c.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
			else if((lX-1024)/width == 3)
			{
				PlaySound(L"d.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
			else if((lX-1024)/width == 4)
			{
				PlaySound(L"e.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
			else if((lX-1024)/width == 5)
			{
				PlaySound(L"f.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
			else if((lX-1024)/width == 6)
			{
				PlaySound(L"g.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
			else
			{
				PlaySound(L"a.wav", NULL, SND_FILENAME | SND_SYNC);
				PlaySound(NULL,NULL, SND_FILENAME | SND_SYNC);	
			}
          } else {
            wsprintf(tszBuffer, TEXT("Finger %d off the pad.\n"), ul);
			PlaySound(NULL, NULL, SND_FILENAME | SND_SYNC);
          }

          switch(ul) {
            case 0: SetDlgItemText(IDC_STATIC1, tszBuffer); break;
            case 1: SetDlgItemText(IDC_STATIC2, tszBuffer); break;
            case 2: SetDlgItemText(IDC_STATIC3, tszBuffer); break;
            case 3: SetDlgItemText(IDC_STATIC4, tszBuffer); break;
            case 4: SetDlgItemText(IDC_STATIC5, tszBuffer); break;
            default: break;
          }
        }

      }
    }
  } else {
      TCHAR tszBuffer[100] = {0};
      wsprintf(tszBuffer, TEXT("This driver installation does not support"));
      SetDlgItemText(IDC_STATIC2, tszBuffer);
      wsprintf(tszBuffer, TEXT("packet group and force processing.\n"));
      SetDlgItemText(IDC_STATIC4, tszBuffer);
  }

  return 0;
}


// Catch WM_CLOSE, some clean up before quit
VOID CSynForcePadDlg::OnClose()
{
  Disconnect();
  delete m_pDevice;
  delete m_pAPI;
  CDialog::DestroyWindow();
  PostQuitMessage(0);
}
