/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "application/GUI/Core/DlgBase.h"
#include "application/GUI/Objects/MenuBar.h"

class MainWindow : public DlgBase
{
public:
    MainWindow();
    ~MainWindow();

    void CreateMenu();

    void Spawn(int show_window);

private:
    bool OnCreateEvent();
    bool OnCloseEvent();

    MenuBar m_menu;
};
