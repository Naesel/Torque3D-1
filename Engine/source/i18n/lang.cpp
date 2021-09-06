//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/stream/stream.h"
#include "core/stream/fileStream.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/ast.h"
#include "console/compiler.h"
#include "core/util/safeDelete.h"
#include "console/engineAPI.h"

#include "i18n/lang.h"

//-----------------------------------------------------------------------------
// LangFile Class
//-----------------------------------------------------------------------------

LangFile::LangFile(const UTF8* langName /* = NULL */, const UTF8* langCode /* = NULL */)
{
   if (langName)
   {
      dsize_t langNameLen = dStrlen(langName) + 1;
      mLangName = new UTF8[langNameLen];
      dStrcpy(mLangName, langName, langNameLen);
   }
   else
      mLangName = NULL;

   if (langCode)
   {
      dsize_t langNameLen = dStrlen(langCode) + 1;
      mLangCode = new UTF8[langNameLen];
      dStrcpy(mLangCode, langCode, langNameLen);
   }
   else
      mLangCode = NULL;
}

LangFile::~LangFile()
{
   SAFE_DELETE_ARRAY(mLangName);
   SAFE_DELETE_ARRAY(mLangCode);
   freeTable();
}

void LangFile::freeTable()
{
   for (typeLocTextHash::Iterator textItr = mTexthash.begin(); textItr != mTexthash.end(); ++textItr)
   {
      delete[] textItr->value;
   }
   mTexthash.clear();
}

UTF8* LangFile::convertColorCodes(UTF8* dst, const UTF8* src)
{
   U32 i = 0;
   U32 j = 0;

   while ('\0' != (dst[j] = src[i]))
   {
      if ((src[i] == '\\') && ((src[i + 1] == 'c') || (src[i + 1] == 'C')) && ((src[i + 2] >= '0') || (src[i + 2] <= '9')))
      {  // Color code found
         dst[j++] = (src[i + 2] - '0') + (UTF8)1;
         i += 3; // Chop off the \cN
      }
      else
         dst[j++] = src[i++];
   }
   return dst;
}

bool LangFile::load(const UTF8* filename)
{
   FileStream* stream;
   if ((stream = FileStream::createAndOpen(filename, Torque::FS::File::Read)) == NULL)
      return false;

   bool ret = load(stream);
   delete stream;
   return ret;
}

bool LangFile::load(Stream* s)
{
   while (s->getStatus() == Stream::Ok)
   {
      char keyBuf[2048], textBuf[2048];
      s->readLongString(2048, keyBuf);
      s->readLongString(2048, textBuf);
      if (s->getStatus() == Stream::Ok)
         addString((const UTF8*)keyBuf, (const UTF8*)textBuf);
   }
   return true;
}

const UTF8* LangFile::getString(StringTableEntry tag)
{
   typeLocTextHash::Iterator textItr = mTexthash.find(tag);
   if (textItr != mTexthash.end())
      return textItr->value;

   return nullptr;
}

void LangFile::addString(const UTF8* tag, const UTF8* str)
{
   StringTableEntry keyPtr = StringTable->insert(tag);
   dsize_t newstrLen = dStrlen(str) + 1;
   UTF8* newstr = new UTF8[newstrLen];
   convertColorCodes(newstr, str);

   // If the string is already in the table, delete the old string.
   typeLocTextHash::Iterator textItr = mTexthash.find(keyPtr);
   if (textItr == mTexthash.end())
      mTexthash.insertUnique(keyPtr, newstr);
   else
   {
      delete[] textItr->value;
      textItr->value = newstr;
   }
}

void LangFile::setLangName(const UTF8* newName)
{
   if (mLangName)
      delete[] mLangName;

   dsize_t langNameLen = dStrlen(newName) + 1;
   mLangName = new UTF8[langNameLen];
   dStrcpy(mLangName, newName, langNameLen);
}

void LangFile::setLangCode(const UTF8* langCode)
{
   if (mLangCode)
      delete[] mLangCode;

   dsize_t codeLen = dStrlen(langCode) + 1;
   mLangCode = new UTF8[codeLen];
   dStrcpy(mLangCode, langCode, codeLen);
}

void LangFile::deactivateLanguage()
{
   if (mLangCode && isLoaded())
      freeTable();
}

//-----------------------------------------------------------------------------
// LangTable Class
//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(LangTable);

ConsoleDocClass(LangTable,
   "@brief Provides the code necessary to handle the low level management "
   "of the string tables for localization\n\n"

   "One LangTable is created for each mod, as well as one for the C++ code. "
   "LangTable is responsible for obtaining the correct strings from each "
   "and relaying it to the appropriate controls.\n\n"

   "@see Localization for a full description\n\n"

   "@ingroup Localization\n"
);

IMPLEMENT_CALLBACK(LangTable, onLoadLanguage, void, (S32 langId, const char* langCode), (langId, langCode),
   "Callback issued to trigger loading of all language files for the passed language code.");

LangTable::LangTable() : mDefaultLang(-1), mCurrentLang(-1)
{
   VECTOR_SET_ASSOCIATION(mLangTable);
}

LangTable::~LangTable()
{
   freeTable();
}

void LangTable::freeTable()
{
   for (S32 i = 0; i < mLangTable.size(); ++i)
   {
      if (mLangTable[i])
         delete mLangTable[i];
   }
   mLangTable.clear();
   mDefaultLang = -1;
   mCurrentLang = -1;
}

bool LangTable::loadTableFromFile(const UTF8* filename)
{
   freeTable();
   FileStream* stream;
   if ((stream = FileStream::createAndOpen(filename, Torque::FS::File::Read)) == NULL)
      return false;

   while (stream->getStatus() == Stream::Ok)
   {
      char codeBuf[256], nameBuf[256];
      stream->readLongString(256, codeBuf);
      stream->readLongString(256, nameBuf);
      if (stream->getStatus() == Stream::Ok)
         addLanguage((const UTF8*)codeBuf, (const UTF8*)nameBuf);
   }

   delete stream;
   return (mLangTable.size() > 0);
}

bool LangTable::saveTableToFile(const UTF8* filename)
{
   FileStream* stream;

   if (mLangTable.size() == 0)
      return false;

   if ((stream = FileStream::createAndOpen(filename, Torque::FS::File::Write)) == NULL)
      return false;

   for (S32 i = 0; i < mLangTable.size(); ++i)
   {
      stream->writeLongString(256, (char*)mLangTable[i]->getLangCode());
      stream->writeLongString(256, (char*)mLangTable[i]->getLangName());
   }

   delete stream;
   return true;
}

S32 LangTable::addLanguage(LangFile* lang)
{
   mLangTable.push_back(lang);

   if (mDefaultLang == -1)
      setDefaultLanguage(mLangTable.size() - 1, false);
   if (mCurrentLang == -1)
      setCurrentLanguage(mLangTable.size() - 1, false);

   return mLangTable.size() - 1;
}

S32 LangTable::addLanguage(const UTF8* langCode, const UTF8* name /* = NULL */)
{
   LangFile* lang = new LangFile(name, langCode);

   S32 ret = addLanguage(lang);
   if (ret >= 0)
      return ret;

   delete lang;
   return -1;
}

bool LangTable::removeLanguage(S32 langid)
{
   if (langid >= 0 && langid < mLangTable.size() && langid != mDefaultLang && langid != mCurrentLang)
   {
      LangFile* pLangFile = mLangTable[langid];
      pLangFile->deactivateLanguage();
      mLangTable.erase(langid);
      delete pLangFile;
      return true;
   }
   return false;
}

const UTF8* LangTable::getString(const char* textTag, bool defaultfallback /* = true */) const
{
   const UTF8* s = NULL;
   StringTableEntry keyPtr = StringTable->insert(textTag);

   if (mCurrentLang >= 0)
      s = mLangTable[mCurrentLang]->getString(keyPtr);
   if (s == NULL && defaultfallback && mDefaultLang >= 0 && mDefaultLang != mCurrentLang)
      s = mLangTable[mDefaultLang]->getString(keyPtr);

   return s;
}

const U32 LangTable::getStringLength(const char* textTag, bool defaultfallback /* = true */) const
{
   const UTF8* s = getString(textTag, defaultfallback);
   if (s)
      return dStrlen(s);

   return 0;
}

void LangTable::setDefaultLanguage(S32 langid, bool activate /* = true */)
{
   if (langid >= 0 && langid < mLangTable.size())
   {
      if ((mDefaultLang >= 0) && (mDefaultLang != langid))
         mLangTable[mDefaultLang]->deactivateLanguage();

      if (activate)
         activateLanguage(langid);

      mDefaultLang = langid;
   }
}

void LangTable::setCurrentLanguage(S32 langid, bool activate /* = true */)
{
   if (langid >= 0 && langid < mLangTable.size())
   {
      if (mCurrentLang >= 0 && mCurrentLang != mDefaultLang && mCurrentLang != langid)
      {
         mLangTable[mCurrentLang]->deactivateLanguage();
         Con::printf("Language %s [%s] deactivated.", mLangTable[mCurrentLang]->getLangName(), mLangTable[mCurrentLang]->getLangCode());
      }

      if (activate)
      {
         activateLanguage(langid);
         Con::printf("Language %s [%s] activated.", mLangTable[langid]->getLangName(), mLangTable[langid]->getLangCode());
      }

      mCurrentLang = langid;
   }
}

void LangTable::activateLanguage(S32 langid)
{
   if (!mLangTable[langid]->isLoaded())
      onLoadLanguage_callback(langid, mLangTable[langid]->getLangCode());
}

bool LangTable::addLocalizedText(S32 langid, const char* filename)
{
   if (langid >= 0 && langid < mLangTable.size())
   {
      S32 startSize = mLangTable[langid]->getNumStrings();
      bool ret = mLangTable[langid]->load(filename);
      S32 newSize = mLangTable[langid]->getNumStrings();

      return ret && (newSize > startSize);
   }

   return false;
}

//-----------------------------------------------------------------------------
// LangTable Console Methods
//-----------------------------------------------------------------------------

DefineEngineMethod(LangTable, loadFromFile, bool, (String filename), ,
   "@brief Loads and initializes the language table from a saved file.\n\n"
   "@param filename Path to a saved language table.\n\n"
   "@return True if the file was found and a valid language table was loaded.\n")
{
   UTF8 scriptFilenameBuffer[1024];

   Con::expandScriptFilename((char*)scriptFilenameBuffer, sizeof(scriptFilenameBuffer), filename);
   return object->loadTableFromFile((const UTF8*)scriptFilenameBuffer);
}

DefineEngineMethod(LangTable, saveToFile, bool, (String filename), ,
   "@brief Saves the language table to the passed file.\n\n"
   "@param filename Path to a save the language table to.\n\n"
   "@return True if the table was not empty and was saved to disk.\n")
{
   UTF8 scriptFilenameBuffer[1024];

   Con::expandScriptFilename((char*)scriptFilenameBuffer, sizeof(scriptFilenameBuffer), filename);
   return object->saveTableToFile((const UTF8*)scriptFilenameBuffer);
}

DefineEngineMethod(LangTable, addLanguage, S32, (String languageCode, String languageName), (""),
   "@brief Adds a language to the table.\n\n"
   "@param languageCode Standard three letter language code (ISO 639-2) "
   "plus two letter country code if relevant (e.g. eng-us, fra, deu, eng-uk).\n"
   "@see References: https://www.loc.gov/standards/iso639-2/php/langcodes-search.php, "
   "https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes \n"
   "@param languageName Optional name to assign to the new language entry.\n"
   "@return The index value for the newly added language or -1 if there was an error.\n")
{
   return object->addLanguage((const UTF8*)languageCode.c_str(), (const UTF8*)languageName.c_str());
}

DefineEngineMethod(LangTable, removeLanguage, bool, (S32 langid), ,
   "@brief Removes a language from the table.\n\n"
   "@param language ID to remove.\n"
   "@note You cannot remove the currently active or default languages."
   "@return True if the language was removed.\n")
{
   return object->removeLanguage(langid);
}

DefineEngineMethod(LangTable, getString, const char*, (const char* textTag, bool defaultFallback), (true),
   "@brief Retrieves a localized text string for the passed tag string.\n\n"
   "@param textTag Text tag to look up the localized string for.\n"
   "@param defaultFallback If the text tag cannot be found in the active language, the default "
   "language will be checked if this is true. Optional, default true.\n"
   "@return A localized text string, \"\" if textTag was not found.\n")
{
   const char* str = (const char*)object->getString(textTag, defaultFallback);
   if (str != NULL)
   {
      dsize_t retLen = dStrlen(str) + 1;
      char* ret = Con::getReturnBuffer(retLen);
      dStrcpy(ret, str, retLen);
      return ret;
   }

   return "";
}

DefineEngineMethod(LangTable, setDefaultLanguage, void, (S32 langId), , "(int language)"
   "@brief Sets the default language table\n\n"
   "@param language ID of the table\n")
{
   object->setDefaultLanguage(langId);
}

DefineEngineMethod(LangTable, setCurrentLanguage, void, (S32 langId), ,
   "(int language)"
   "@brief Sets the current language table for grabbing text\n\n"
   "@param language ID of the table\n")
{
   object->setCurrentLanguage(langId);
}

DefineEngineMethod(LangTable, getCurrentLanguage, S32, (), , "()"
   "@brief Get the ID of the current language table\n\n"
   "@return Numerical ID of the current language table")
{
   return object->getCurrentLanguage();
}

DefineEngineMethod(LangTable, getLangCode, const char*, (S32 langId), ,
   "@brief Return the ISO 639-1 or 639-2 code assigned for the language. "
   "This is usually the filename for the compiled text files of this language.\n\n"
   "@param langId Numerical index of the language table to access.\n\n"
   "@return String containing the language code, NULL if langId was invalid.")
{
   const char* str = (const char*)object->getLangCode(langId);
   if (str != NULL)
   {
      dsize_t retLen = dStrlen(str) + 1;
      char* ret = Con::getReturnBuffer(retLen);
      dStrcpy(ret, str, retLen);
      return ret;
   }

   return "";
}

DefineEngineMethod(LangTable, getLangName, const char*, (S32 langId), ,
   "@brief Return the readable name of the language.\n\n"
   "@param langId Numerical index of the language to access.\n\n"
   "@return String containing the name of the language, NULL if langId was invalid or name was never specified")
{
   const char* str = (const char*)object->getLangName(langId);
   if (str != NULL)
   {
      dsize_t retLen = dStrlen(str) + 1;
      char* ret = Con::getReturnBuffer(retLen);
      dStrcpy(ret, str, retLen);
      return ret;
   }

   return "";
}

DefineEngineMethod(LangTable, getNumLang, S32, (), , "()"
   "@brief Used to find out how many languages are in the table\n\n"
   "@return Size of the vector containing the languages, numerical")
{
   return object->getNumLang();
}

DefineEngineMethod(LangTable, addLocalizedText, bool, (S32 langid, const char* langFile), , "()"
   "@brief Load a compiled localized text file into the selected language.\n\n"
   "@param langid Numerical index of the language to access.\n"
   "@param langFile Path to the lso file to load text from.\n"
   "@return True if the file was found and text was added to the LangFile.\n")
{
   return object->addLocalizedText(langid, langFile);
}

//-----------------------------------------------------------------------------
// Support Functions
//-----------------------------------------------------------------------------

UTF8* sanitiseVarName(const UTF8* varName, UTF8* buffer, U32 bufsize)
{
   if (!varName || bufsize < 10)	// [tom, 3/3/2005] bufsize check gives room to be lazy below
   {
      *buffer = 0;
      return NULL;
   }

   dStrcpy(buffer, (const UTF8*)"I18N::", bufsize);

   UTF8* dptr = buffer + 6;
   const UTF8* sptr = varName;
   while (*sptr)
   {
      if (dIsalnum(*sptr))
         *dptr++ = *sptr++;
      else
      {
         if (*(dptr - 1) != '_')
            *dptr++ = '_';
         sptr++;
      }

      if ((dptr - buffer) >= (bufsize - 1))
         break;
   }
   *dptr = 0;

   return buffer;
}

UTF8* getCurrentModVarName(UTF8* buffer, U32 bufsize)
{
   char varName[256];
   StringTableEntry cbName = CodeBlock::getCurrentCodeBlockName();

   const UTF8* slash = (const UTF8*)dStrchr(cbName, '/');
   if (slash == NULL)
   {
      Con::errorf("Illegal CodeBlock path detected in sanitiseVarName() (no mod directory): %s", cbName);
      return NULL;
   }

   dStrncpy(varName, cbName, slash - (const UTF8*)cbName);
   varName[slash - (const UTF8*)cbName] = 0;

   return sanitiseVarName((UTF8*)varName, buffer, bufsize);
}

const LangTable* getCurrentModLangTable()
{
   UTF8 saneVarName[256];

   if (getCurrentModVarName(saneVarName, sizeof(saneVarName)))
   {
      const LangTable* lt = dynamic_cast<LangTable*>(Sim::findObject(Con::getIntVariable((const char*)saneVarName)));
      return lt;
   }
   return NULL;
}

const LangTable* getModLangTable(const UTF8* mod)
{
   UTF8 saneVarName[256];

   if (sanitiseVarName(mod, saneVarName, sizeof(saneVarName)))
   {
      const LangTable* lt = dynamic_cast<LangTable*>(Sim::findObject(Con::getIntVariable((const char*)saneVarName)));
      return lt;
   }
   return NULL;
}

//lang_ localization
bool compiledFileNeedsUpdate(UTF8* filename)
{
   Torque::Path filePath = Torque::Path(filename);
   Torque::FS::FileNodeRef sourceFile = Torque::FS::GetFileNode(filePath);
   Torque::Path compiledPath = Torque::Path(filePath);
   compiledPath.setExtension("lso");
   Torque::FS::FileNodeRef compiledFile = Torque::FS::GetFileNode(compiledPath);

   Torque::Time sourceModifiedTime, compiledModifiedTime;

   if (sourceFile != NULL)
      sourceModifiedTime = sourceFile->getModifiedTime();

   if (compiledFile != NULL)
      compiledModifiedTime = compiledFile->getModifiedTime();

   if (sourceModifiedTime > compiledModifiedTime)
      return true;
   return false;
}

DefineEngineFunction(CompileLanguage, void, (const char* inputFile), ,
   "@brief Compiles a LSO language file.\n"
   "@param inputFile Path to the .txt file to compile. The output .lso file "
   "will be placed in the same directory and have the same name.\n"
   "The input file must follow this example layout:\n"
   "txt_hello_world = Hello world in english!\n"
   "The text to the left of the equal sign is the text id. Text to the right of "
   "the equal sign is the localized text. There must be a single space on each "
   "side of the equal sign.\n")
{
   UTF8 scriptFilenameBuffer[1024];
   Con::expandScriptFilename((char*)scriptFilenameBuffer, sizeof(scriptFilenameBuffer), inputFile);

   if (!Torque::FS::IsFile(scriptFilenameBuffer))
   {
      Con::errorf("CompileLanguage - file %s not found", scriptFilenameBuffer);
      return;
   }

   if (!compiledFileNeedsUpdate(scriptFilenameBuffer))
      return;

   FileObject file;
   if (!file.readMemory(scriptFilenameBuffer))
   {
      Con::errorf("CompileLanguage - couldn't read file %s", scriptFilenameBuffer);
      return;
   }

   FileStream* outStream;
   Torque::Path lsoPath = scriptFilenameBuffer;
   lsoPath.setExtension("lso");

   if ((outStream = FileStream::createAndOpen(lsoPath.getFullPath(), Torque::FS::File::Write)) == NULL)
   {
      Con::errorf("Could not open output file (%s) for compiled language", lsoPath.getFullPath().c_str());
      return;
   }
   Con::printf("Compiling language file: %s.", lsoPath.getFullPath().c_str());

   const U8* inLine = NULL;
   const char* separatorStr = " = ";
   while (!file.isEOF())
   {
      inLine = file.readLine();
      char* line;
      chompUTF8BOM((const char*)inLine, &line);

      // Skip comments and blank lines
      if ((line[0] == '/') || (line[0] == '#') || (dStrlen(line) == 0))
         continue;

      char* div = dStrstr(line, separatorStr);
      if (div == NULL)
      {
         Con::errorf("Separator %s not found in line: %s", separatorStr, line);
         Con::errorf("Could not determine string name ID");
         continue;
      }
      *div = 0;
      char* text = div + dStrlen(separatorStr);

      if (dStrlen(text) > 2048)
      {
         text[2047] = '\0';
         Con::warnf("The following localized text has been truncated to 2048 characters:\n%s", text);
      }

      outStream->writeLongString(2048, line);
      outStream->writeLongString(2048, text);
   }

   outStream->close();
   delete outStream;
}

DefineEngineStringlyVariadicFunction(buildString, const char*, 2, 11, "(string format, ...)"
   "@brief Build a string from a format string. This function is identical to 'buildTaggedString'"
   "for plain text strings that are not network tag IDs.\n\n"

   "This function takes a format string and one "
   "or more additional strings.  If the format string contains argument tags that range from "
   "%%1 through %%9, then each additional string will be substituted into the format string.  "
   "The final combined string will be returned.  The maximum length of the format "
   "string plus any inserted additional strings is 511 characters.\n\n"

   "@param format A string that contains zero or more argument tags, in the form of "
   "%%1 through %%9.\n"
   "@param ... A variable number of arguments that are insterted into the tagged string "
   "based on the argument tags within the format string.\n"

   "@returns A string that is a combination of the original format string with any additional "
   "strings passed in inserted in place of each argument tag.\n")
{
   const char* fmtString = argv[1];
   static const U32 bufSize = 512;
   char* strBuffer = Con::getReturnBuffer(bufSize);
   const char* fmtStrPtr = fmtString;
   char* strBufPtr = strBuffer;
   S32 strMaxLength = bufSize - 1;
   if (!fmtString)
      goto done;

   //build the string
   while (*fmtStrPtr)
   {
      //look for an argument tag
      if (*fmtStrPtr == '%')
      {
         if (fmtStrPtr[1] >= '1' && fmtStrPtr[1] <= '9')
         {
            S32 argIndex = S32(fmtStrPtr[1] - '0') + 1;
            if (argIndex >= argc)
               goto done;
            const char* argStr = argv[argIndex];
            if (!argStr)
               goto done;
            S32 strLength = dStrlen(argStr);
            if (strLength > strMaxLength)
               goto done;
            dStrcpy(strBufPtr, argStr, strMaxLength);
            strBufPtr += strLength;
            strMaxLength -= strLength;
            fmtStrPtr += 2;
            continue;
         }
      }

      //if we don't continue, just copy the character
      if (strMaxLength <= 0)
         goto done;
      *strBufPtr++ = *fmtStrPtr++;
      strMaxLength--;
   }

done:
   *strBufPtr = '\0';
   return strBuffer;
}

//end lang_ localization
