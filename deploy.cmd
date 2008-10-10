set VM="c:\data\vm\Win2k3\Windows Server 2003 Enterprise Edition.vmx"
set BINDIR=c:\nk\compiled
set DEPDIR=c:\nk

vmrun revertToSnapshot %VM% ForDeploy
vmrun start %VM%

vmrun -gu Administrator -gp 1q2w3e runProgramInGuest %VM% "%windir%\system32\cmd.exe" "/c mkdir \"%DEPDIR%\""

vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\nkfilter.sys" "%DEPDIR%\nkfilter.sys"
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\nkfilter.inf" "%DEPDIR%\nkfilter.inf"
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\nkcontrol.exe" "%DEPDIR%\nkcontrol.exe"
vmrun -gu Administrator -gp 1q2w3e copyFileFromHostToGuest %VM% "%BINDIR%\psexec.exe" "%DEPDIR%\psexec.exe"

vmrun -gu Administrator -gp 1q2w3e runProgramInGuest %VM% "%DEPDIR%\psexec.exe" -accepteula -i -w "%DEPDIR%" "%DEPDIR%\nkcontrol.exe" install
vmrun reset %VM% soft
