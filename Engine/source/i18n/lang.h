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

//-----------------------------------------------------------------------------
/// \file lang.h
/// \brief Header for language support
//-----------------------------------------------------------------------------

#include "console/simBase.h"
#include "core/util/tDictionary.h"
//lang_ localization
#include "core/fileObject.h"
#include "core/util/str.h"
#include "core/strings/unicode.h"

#ifndef _LANG_H_
#define _LANG_H_

#define LANG_INVALID_ID		0xffffffff		///!< Invalid ID. Used for returning failure

//-----------------------------------------------------------------------------
/// \brief Class for working with language files
//-----------------------------------------------------------------------------
class LangFile
{
protected:
   typedef HashTable<StringTableEntry, const char*> typeLocTextHash;
   typeLocTextHash mTexthash;
   UTF8* mLangName;
   UTF8* mLangCode;

   void freeTable();

   // Convert color codes in localized text
   UTF8* convertColorCodes(UTF8* dst, const UTF8* src);

public:
   LangFile(const UTF8* langName = NULL, const UTF8* langCode = NULL);
   virtual ~LangFile();

   bool load(const UTF8* filename);

   bool load(Stream* s);

   const UTF8* getString(StringTableEntry tag);
   void addString(const UTF8* tag, const UTF8* str);

   void setLangName(const UTF8* newName);
   void setLangCode(const UTF8* newCode);
   const UTF8* getLangName(void) { return mLangName; }
   const UTF8* getLangCode(void) { return mLangCode; }

   void deactivateLanguage(void);
   bool isLoaded(void) { return mTexthash.size() > 0; }
   S32 getNumStrings(void) { return mTexthash.size(); }
};

//-----------------------------------------------------------------------------
/// \brief Language file table
//-----------------------------------------------------------------------------
class LangTable : public SimObject
{
   typedef SimObject Parent;

protected:
   Vector<LangFile*> mLangTable;
   S32 mDefaultLang;
   S32 mCurrentLang;

public:
   DECLARE_CALLBACK(void, onLoadLanguage, (S32 langId, const char* langCode));
   DECLARE_CONOBJECT(LangTable);

   LangTable();
   virtual ~LangTable();

   bool loadTableFromFile(const UTF8* filename);
   bool saveTableToFile(const UTF8* filename);

   void freeTable();
   S32 addLanguage(LangFile* lang);
   S32 addLanguage(const UTF8* langCode, const UTF8* name = NULL);
   bool removeLanguage(S32 langid);

   void setDefaultLanguage(S32 langid, bool activate = true);
   void setCurrentLanguage(S32 langid, bool activate = true);
   S32 getCurrentLanguage(void) { return mCurrentLang; }
   void activateLanguage(S32 langid);
   bool addLocalizedText(S32 langid, const char* filename);

   const UTF8* getLangName(const S32 langid) const
   {
      if (langid < 0 || langid >= mLangTable.size())
         return NULL;
      return mLangTable[langid]->getLangName();
   }

   const UTF8* getLangCode(const S32 langid) const
   {
      if (langid < 0 || langid >= mLangTable.size())
         return NULL;
      return mLangTable[langid]->getLangCode();
   }

   const S32 getNumLang(void) const { return mLangTable.size(); }

   const UTF8* getString(const char* textTag, bool defaultfallback = true) const;
   const U32 getStringLength(const char* textTag, bool defaultfallback = true) const;
};

extern UTF8 *sanitiseVarName(const UTF8 *varName, UTF8 *buffer, U32 bufsize);
extern UTF8 *getCurrentModVarName(UTF8 *buffer, U32 bufsize);
extern const LangTable *getCurrentModLangTable();
extern const LangTable *getModLangTable(const UTF8 *mod);

#endif // _LANG_H_
