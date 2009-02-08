set VM="d:\vm\Win2k3\Windows Server 2003 Enterprise Edition.vmx"
set REPODIR=d:\gitrepos\nk
set BINDIR=%REPODIR%\compiled
set DEPDIR=c:\nk

:: Load test snapshot
vmrun revertToSnapshot %VM% ForDeployVirtualDisk
vmrun start %VM%

:: Create deployment folder
vmrun -gu Administrator -gp 1q2w3e runProgramInGuest %VM% "%windir%\system32\cmd.exe" "/c mkdir \"%DEPDIR%\""

:: Copy files to VM
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\nkfilter.sys" "%DEPDIR%\nkfilter.sys"
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\nkfilter.inf" "%DEPDIR%\nkfilter.inf"
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\nkcontrol.exe" "%DEPDIR%\nkcontrol.exe"
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%REPODIR%\third\psexec.exe" "%DEPDIR%\psexec.exe"

:: Install nkfilter
vmrun -gu Administrator -gp 1q2w3e runProgramInGuest %VM% "%DEPDIR%\psexec.exe" -accepteula -i -w "%DEPDIR%" "%DEPDIR%\nkcontrol.exe" install

:: Reboot VM
vmrun -gu Administrator -gp 1q2w3e runProgramInGuest %VM% "%DEPDIR%\psexec.exe" -accepteula shutdown /r /t 0
