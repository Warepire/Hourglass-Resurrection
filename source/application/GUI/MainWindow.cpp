/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "application/GUI/Core/DlgBase.h"
#include "MainWindow.h"

enum MainWindowIDs
{
    IDC_STATIC,
    IDC_RADIO_READONLY,
    IDC_RADIO_READWRITE,
    IDC_TEXT_MOVIE,
    IDC_BUTTON_MOVIEBROWSE,
    IDC_EDIT_FPS,
    IDC_EDIT_CURFRAME,
    IDC_EDIT_MAXFRAME,
    IDC_STATIC_FRAMESLASH,
    IDC_EDIT_RERECORDS,
    IDC_STATIC_MOVIETIME,
    IDC_STATIC_CURRENTFPS,
    IDC_TEXT_EXE,
    IDC_BUTTON_GAMEBROWSE,
    IDC_EDIT_SYSTEMCLOCK,
    IDC_STATIC_MOVIESTATUS,
    IDC_BUTTON_RECORD,
    IDC_BUTTON_PLAY,
    IDC_BUTTON_STOP,
    IDC_PAUSED,
    IDC_RADIO_THREAD_ALLOW,
    IDC_RADIO_THREAD_WRAP,
    IDC_RADIO_THREAD_DISABLE,
    IDC_CHECK_MUTE,
    IDC_FASTFORWARD,
    IDC_EDIT_COMMANDLINE,
};

MainWindow::MainWindow() :
    DlgBase(
        L"Hourglass-Resurrection"
#ifdef _DEBUG
        L" (debug)"
#endif
        , 0, 0, 301, 178, DlgType::NORMAL)
{
    //LTEXT           "", IDC_TOPLEFTCONTROL, 0, 0, 1, 1
    AddRadioButton(L"Read-Only", IDC_RADIO_READONLY, 235, 44, 50, 10, false, false);
    AddRadioButton(L"Read+Write", IDC_RADIO_READWRITE, 235, 56, 54, 10, false, true);
    AddEditControl(IDC_TEXT_MOVIE, 7, 13, 230, 14, true, false);
    AddStaticText(L"Movie File", IDC_STATIC, 10, 3, 32, 8);
    AddPushButton(L"Browse...", IDC_BUTTON_MOVIEBROWSE, 246, 13, 48, 14, false, false);
    AddStaticText(L"Frame:", IDC_STATIC, 12, 32, 27, 8);
    AddStaticText(L"Re-records:", IDC_STATIC, 49, 61, 39, 8);
    AddStaticText(L"Frames per Second:", IDC_STATIC, 21, 47, 80, 8);
    AddEditControl(IDC_EDIT_FPS, 91, 45, 40, 12, false, false);
    AddEditControl(IDC_EDIT_CURFRAME, 38, 30, 40, 12, false, false);
    AddEditControl(IDC_EDIT_MAXFRAME, 91, 30, 40, 12, false, false);
    AddStaticText(L"/", IDC_STATIC_FRAMESLASH, 83, 32, 8, 8);
    AddEditControl(IDC_EDIT_RERECORDS, 91, 59, 40, 12, false, false);
    AddStaticText(L"Time: 0h 5m 32.15s   /   0h 16m 44.12s", IDC_STATIC_MOVIETIME, 144, 32, 149, 8);
    AddStaticText(L"Current FPS: 0", IDC_STATIC_CURRENTFPS, 144, 47, 82, 8);
    AddEditControl(IDC_TEXT_EXE, 7, 134, 230, 14, true, false);
    AddStaticText(L"Game Executable", IDC_STATIC, 10, 125, 56, 8);
    AddPushButton(L"Browse...", IDC_BUTTON_GAMEBROWSE, 246, 134, 48, 14, false, false);
    AddStaticText(L"System Time: ", IDC_STATIC, 21, 79, 66, 8);
    AddEditControl(IDC_EDIT_SYSTEMCLOCK, 71, 77, 60, 12, false, false);
    AddStaticText(L"Current Status: Playing", IDC_STATIC_MOVIESTATUS, 144, 61, 91, 8);
    AddPushButton(L"Run and Record New Movie", IDC_BUTTON_RECORD, 8, 154, 99, 17, true, false);
    AddPushButton(L"Run and Play Existing Movie", IDC_BUTTON_PLAY, 192, 154, 102, 17, true, false);
    AddPushButton(L"Stop Running", IDC_BUTTON_STOP, 116, 154, 67, 17, true, false);
    AddCheckbox(L"Paused", IDC_PAUSED, 236, 69, 39, 10, false);
    AddGroupBox(L"Multithreading and Wait Sync", IDC_STATIC, 13, 94, 118, 23);
    AddRadioButton(L"Allow", IDC_RADIO_THREAD_ALLOW, 20, 103, 33, 10, false, false);
    AddRadioButton(L"Wrap", IDC_RADIO_THREAD_WRAP, 54, 103, 33, 10, false, true);
    AddRadioButton(L"Disable", IDC_RADIO_THREAD_DISABLE, 88, 103, 39, 10, false, true);
    AddCheckbox(L"Mute", IDC_CHECK_MUTE, 246, 108, 39, 10, false);
    AddCheckbox(L"Fast-Forward", IDC_FASTFORWARD, 236, 82, 57, 10, false);
    AddStaticText(L"Command Line Arguments", IDC_STATIC, 143, 94, 97, 8);
    AddEditControl(IDC_EDIT_COMMANDLINE, 140, 104, 97, 12, false, false);
}

MainWindow::~MainWindow()
{
}

void MainWindow::Spawn(int show_window)
{
    SpawnDialogBox(nullptr, DlgMode::INDIRECT);
    ShowDialogBox(show_window);
    UpdateWindow();
}
