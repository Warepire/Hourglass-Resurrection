#include "CheckableMenuItem.h"

CheckableMenuItem::CheckableMenuItem(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg) :
    MenuItemBase(title, shortcut, false, parent, dlg)
{
}

CheckableMenuItem& CheckableMenuItem::SetEnabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Unset);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::SetDisabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Set);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::SetChecked()
{
    SetUnsetStyleBits(MFS_CHECKED, SetBits::Set);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::SetUnchecked()
{
    SetUnsetStyleBits(MFS_CHECKED, SetBits::Unset);
    return *this;
}
