#include "SubMenu.h"

SubMenu::SubMenu(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg) :
    MenuItemBase(title, shortcut, true, parent, dlg)
{
}

SubMenu& SubMenu::SetEnabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Unset);
    return *this;
}

SubMenu& SubMenu::SetDisabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Set);
    return *this;
}
