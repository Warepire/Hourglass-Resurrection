/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstddef>
#include <vector>

class MenuBase
{
protected:
    MenuBase(std::size_t size);

    std::vector<BYTE> m_menu;
    std::vector<MenuBase*> m_children;

    friend class DlgBase;
    friend class MenuItemBase;
};
