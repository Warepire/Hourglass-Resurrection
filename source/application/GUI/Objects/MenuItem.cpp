#include "MenuItem.h"

MenuItem::MenuItem(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg) :
    MenuItemBase(title, shortcut, false, parent, dlg)
{
}

MenuItem& MenuItem::SetEnabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Unset);
    return *this;
}

MenuItem& MenuItem::SetDisabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Set);
    return *this;
}
