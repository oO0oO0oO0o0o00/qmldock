exit 0

python -m venv venv
venv\Scripts\activate.bat
echo %VIRTUAL_ENV%
pip install --index-url=https://download.qt.io/official_releases/QtForPython/ --trusted-host download.qt.io shiboken6 pyside6 shiboken6_generator