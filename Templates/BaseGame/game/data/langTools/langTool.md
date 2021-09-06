#Adding A Language

Clone https://github.com/OTHGMars/Torque3D-1/tree/localize
Build the engine and install the BaseGame template.
Click the 'Language Tool' button on the main menu.
Click 'Add' in the tool gui.
In the popup gui enter the three letter ISO 639-2 code for the language you are adding. Codes can be found here: https://www.loc.gov/standards/iso639-2/php/langcodes-search.php. Using the standard code is helpful because it will allow language to be cross-referenced when automatically generating manifests for localized external APIs (Steam, OpenVR, etc.).
Enter the localized name for the language as it should appear in the language selector popup and click ok.
The new language should now show in the list in the tool gui. Select it and click 'Update'. This will generate all of the .txt files that need translated to fully implement the language. Open the console to see the file names and locations. 
Fill in all missing text in the .txt files identified above. All lines beginning with '/' or '#' are comments and will not be included in the compiled language text. Comments that begin with '/' are user added in the default language to understand the context for the text. Comments that begin with '#' are automatically added to show the text value of the default language (English) above each tag that needs text filled in.
After all of the missing text has been filled in, re-launch the game and the new files will be automatically compiled and the language can be tested by selecting it in the language selector (on MainMenu).

###After testing, please PR your language.
Copy data/language/langTable.lso from your game directory to the template. This is the language table that contains the names and codes for all of the available languages.
Copy all of the .txt files that you edited to the corresponding 'lang' folders under 'Templates'.
Commit to your own branch and PR to https://github.com/OTHGMars/Torque3D-1/tree/localize