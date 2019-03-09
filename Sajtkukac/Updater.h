/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * Updater.h is part of Sajtkukac.
 *
 * Sajtkukac is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Sajtkukac is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Sajtkukac. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#define WM_USER_UPDATER (WM_USER + 5)
#define WM_USER_UPDATER_FAIL (WM_USER + 6)
#define WM_USER_UPDATER_NOT_FOUND (WM_USER + 7)
#define WM_USER_UPDATER_FOUND (WM_USER + 8)
#define WM_USER_UPDATER_DONE (WM_USER + 9)

BOOL Updater(HWND, LPARAM, BOOL);