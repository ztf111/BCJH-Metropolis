@REM *********************
@REM ** ��һ���������޸� **
@REM *********************

@REM When adding lines, remember to update the README file! Two additional lines need to be added.
@REM This file should be encoded in GB2312.
echo ����������https://github.com/hjenryin/BCJH-Metropolis/discussions/categories/q-a ����
echo ���κ��뷨����https://github.com/hjenryin/BCJH-Metropolis/discussions/categories/general ����
echo ������������⣬��ӭ�� https://github.com/hjenryin/BCJH-Metropolis/issues/new ���~

powershell -command "$file = 'ruleData.json'; $halfHourAgo = (Get-Date).AddMinutes(-30); $modifiedTime = (Get-Item $file).LastWriteTime; $modifiedTime -lt $halfHourAgo" | findstr "True" >NUL && call:download_data || echo ��Сʱ���Ѹ��¹�������������

set "bcjhid="
set /p bcjhid="�����Ҫ���������û����ã���������Ϸ����λУ���루���Ͻ�ͷ��->����->�ײ˾ջ���ע�ⲻ�ǰײ˾ջ����ϴ�������ID��������ֱ�ӻس���"

if "%bcjhid%"=="" (
    echo δ����id�����������û�����
) 

if not "%bcjhid%"=="" (
    echo ���������û�����
    @REM �����û�����
    powershell -command curl -o "directUserData.json" "https://yx518.com/api/archive.do?token=%bcjhid%"
    @REM ����Ƿ����سɹ�
    powershell -command $json=$(iwr -uri "https://yx518.com/api/archive.do?token=%bcjhid%" ^| ConvertFrom-Json^) ^; if ($json.ret -eq 'S'^) { Write-Host ���سɹ� -ForegroundColor:Green } else { Write-Host ����ʧ�ܣ�����id�Ƿ���ȷ -BackgroundColor:Red -ForegroundColor:White ^}
    win-iconv\iconv.exe -f utf-8 -t gbk directUserData.json > tmp.json
    del directUserData.json
    ren tmp.json directUserData.json
)
echo=
@REM ****************************************************************************************************
@REM ** ������Ҫ�������ڴ��޸�-C��-R������������ĵ����������Լ�--target����������������Ŀ�� **
@REM ****************************************************************************************************

.\bcjh.exe -C 5000 -R 1000 --target 5000000
@REM -h ����


@REM *********************
@REM ** ��һ���������޸� **
@REM *********************
echo ��������ر�
pause >NUL
goto :eof


:download_data
powershell -command "Write-Host '���ڻ�ȡ������... ' -NoNewline"
powershell -command curl -o "ruleData.json" "https://bcjh.xyz/api/get_banquet_rule"
win-iconv\iconv.exe -f utf-8 -t gbk ruleData.json > tmp 
if %ERRORLEVEL% EQU 0 (
    del ruleData.json
    ren tmp ruleData.json
    echo �������ȡ���
) else (
    echo �������ȡʧ�ܣ�ʹ���ѻ������������������������£��������粢���ԡ�
)

powershell -command "Write-Host '���ڻ�ȡ���³�ʦ����... ' -NoNewline"
powershell -command curl -o "data.min.json" "https://yuwenxifan.github.io/bcjhMobile/data/data.min.json"
win-iconv\iconv.exe -f utf-8 -t gbk data.min.json > tmp 
if %ERRORLEVEL% EQU 0 (
    del data.min.json
    ren tmp data.min.json
    echo ��ʦ���׻�ȡ���
) else (
    echo ��ʦ���׻�ȡʧ�ܣ�ʹ���ѻ���ĳ������ݡ�������ݲ������£��������粢���ԡ�
)
goto :eof
