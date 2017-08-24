#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/MenuItemBase.h"

class MenuItem : public MenuItemBase
{
public:
    MenuItem(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg);

    MenuItem& SetEnabled();
    MenuItem& SetDisabled();
};
