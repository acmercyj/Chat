@echo off

echo ----------------------------------------------------
echo 清理这些垃圾东西
echo Press any key to delete all files with ending:
echo  *.aps *.idb *.ncp *.obj *.pch *.sbr *.tmp *.pdb *.bsc *.ilk *.res *.ncb *.opt *.suo *.manifest *.dep *.o *.user *.log
echo Visual c++/.Net junk 
echo ----------------------------------------------------
pause

del /F/Q/S *.aps *.idb *.ncp *.obj *.pch *.sbr *.tmp *.pdb *.bsc *.ilk *.res *.ncb *.opt *.suo *.manifest *.dep *.o *.user *.log
del /a:h/F/Q/S *.aps *.idb *.ncp *.obj *.pch *.sbr *.tmp *.pdb *.bsc *.ilk *.res *.ncb *.opt *.suo *.manifest *.dep *.o *.user *.log

pause


