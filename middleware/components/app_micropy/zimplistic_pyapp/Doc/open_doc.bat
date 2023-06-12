@echo off
if exist Itor3_document\index.html goto openDoc
echo There is no document generated for Itor3 code yet
echo Please run "gen_doc.bat" to generate Itor3 code document
echo.
echo Press any key to exit...
pause > nul
exit

:openDoc
start Itor3_pyapp_document\index.html
