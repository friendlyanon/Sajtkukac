/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * updater.js is part of Sajtkukac.
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

/*@cc_on
@if (@_jscript)
  var fso = WScript.CreateObject('Scripting.FileSystemObject'),
      shell = WScript.CreateObject('Shell.Application'),
      wShell = WScript.CreateObject('WScript.Shell'),
      zipFile = fso.GetAbsolutePathName('./_update.zip'),
      outDir = fso.GetAbsolutePathName('./'),
      filesInZip;
  WScript.Sleep(2000);
  try {
    filesInZip = shell.NameSpace(zipFile).Items();
    shell.NameSpace(outDir).CopyHere(filesInZip, 16);
    fso.DeleteFile(zipFile);
    wShell.Run('Sajtkukac.exe', 0, false);
  }
  catch (e) {
    wShell.Popup(e.message, 0, 'Error', 1 << 4);
  }
  WScript.Quit();
@else @*/
console.log('Please do not run this file manually.');
/*@end @*/
