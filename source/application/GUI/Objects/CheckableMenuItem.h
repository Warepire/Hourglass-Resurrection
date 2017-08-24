#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/MenuItemBase.h"

class CheckableMenuItem : public MenuItemBase
{
public:
    CheckableMenuItem(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg);

    CheckableMenuItem& SetEnabled();
    CheckableMenuItem& SetDisabled();

    CheckableMenuItem& SetChecked();
    CheckableMenuItem& SetUnchecked();
};
