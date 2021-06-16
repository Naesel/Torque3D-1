//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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


#include "webEngine.h"
#include "include/cef_parser.h"
#include "platform/threads/mutex.h"

// ClientFileHandlerFactory methods:
//------------------------------------------------------------------------------
// Return a new scheme handler instance to handle the request.
CefRefPtr<CefResourceHandler> ClientFileHandlerFactory::Create(CefRefPtr<CefBrowser> browser,
   CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request)
{
   CEF_REQUIRE_IO_THREAD();

   return new FileRequestHandler();
}

//------------------------------------------------------------------------------
FileRequestHandler::~FileRequestHandler()
{
   if ((_file_stream.getStatus() == Stream::Ok) || (_file_stream.getStatus() == Stream::EOS))
      _file_stream.close();
}

//------------------------------------------------------------------------------
bool FileRequestHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback)
{  // To handle the request immediately set |handle_request| to true and return true.
   // To cancel the request immediately set |handle_request| to true and return false.
   // To decide at a later time set |handle_request| to false, return true, and execute
   // |callback| to continue or cancel the request.

   // Save the uri and callback so we can open later.
   _file_uri = CefURIDecode(request->GetURL(), false, static_cast<cef_uri_unescape_rule_t>(UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS));
   _callback_ptr = callback;

   // Add to the pending open vector so files get opened on the T3D main thread.
   Mutex::lockMutex(gWebEngine->getResourceMutex());
   gWebEngine->addResourceHandler(this);
   Mutex::unlockMutex(gWebEngine->getResourceMutex());
   handle_request = false;
   return true;
}

//------------------------------------------------------------------------------
void FileRequestHandler::GetResponseHeaders(CefRefPtr<CefResponse> response,
   int64& response_length, CefString& redirectUrl)
{
   CEF_REQUIRE_IO_THREAD();

   // Save the stream size and set it as the response length.
   _data_size = _file_stream.getStreamSize();
   response_length = _data_size;

   // Set mimetype if it can be guessed, otherwise leave blank.
   if (dStristr(_file_path.getExtension().c_str(), "css") != 0)
      _mime_type = "text/css";
   else if (dStristr(_file_path.getExtension().c_str(), "js") != 0)
      _mime_type = "text/javascript";
   else if (dStristr(_file_path.getExtension().c_str(), "png") != 0)
      _mime_type = "image/png";
   else if (dStristr(_file_path.getExtension().c_str(), "jpg") != 0)
      _mime_type = "image/jpg";
   else if (dStristr(_file_path.getExtension().c_str(), "html") != 0)
      _mime_type = "text/html";
   else
      _mime_type = String::EmptyString;
   response->SetMimeType(_mime_type.c_str());

   // The HTTP 200 OK success status response code indicates that the request has succeeded.
   // https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/200
   response->SetStatus(200);
}

//------------------------------------------------------------------------------
bool FileRequestHandler::Skip(int64 bytes_to_skip, int64& bytes_skipped,
   CefRefPtr<CefResourceSkipCallback> callback)
{
   if( (_file_stream.getStatus() == Stream::Ok) && (_offset < _data_size))
   {
      if ((_offset + bytes_to_skip) < _data_size)
      {
         _offset += bytes_to_skip;
         bytes_skipped = bytes_to_skip;
      }
      else
      {
         bytes_skipped = _data_size - _offset;
         _offset = _data_size;
      }

      _file_stream.setPosition((U32)_offset);
      return true;
   }

   bytes_skipped = -2;
   return false;
}

//------------------------------------------------------------------------------
bool FileRequestHandler::Read(void* data_out, int bytes_to_read, int& bytes_read,
   CefRefPtr<CefResourceReadCallback> callback)
{
   bytes_read = 0;
   if ((_file_stream.getStatus() == Stream::Ok) && (_offset < _data_size))
   {  // If data is available immediately copy up to |bytes_to_read| bytes into
      // |data_out|, set |bytes_read| to the number of bytes copied, and return true.

      // bytes_to_read is not limited to the file size reported in GetResponseHeaders.
      // Don't attempt to read past the end of stream.
      U32 numBytes = (_offset + bytes_to_read) > _data_size ? (U32) (_data_size - _offset) : bytes_to_read;
      if (_file_stream.read(numBytes, data_out))
      {
         bytes_read = numBytes;
         _offset += bytes_read;
         return true;
      }
   }
   else if (_offset == _data_size)
   {  // To indicate response completion set |bytes_read| to 0 and return false.
      return false;
   }

   // To indicate failure set |bytes_read| to < 0 (e.g. -2 for ERR_FAILED) and return false.
   bytes_read = -2;
   return false;
}

//------------------------------------------------------------------------------
void FileRequestHandler::Cancel()
{
   CEF_REQUIRE_IO_THREAD();

   _offset = 0;
   _data_size = 0;

   if ((_file_stream.getStatus() == Stream::Ok) || (_file_stream.getStatus() == Stream::EOS))
   {
      _file_stream.close();
   }
}

//------------------------------------------------------------------------------
// Called on T3D main thread.
void FileRequestHandler::tryOpenFile()
{
   bool doContinue = false;
   if (uriToFilePath(_file_uri))
   {  // File found
      _offset = 0;
      _data_size = 0;

      _file_stream.open(_file_path.getFullPath(), Torque::FS::File::Read);
      doContinue = _file_stream.getStatus() == Stream::Ok;
   }

   if (doContinue)
      _callback_ptr->Continue();
   else
      _callback_ptr->Cancel();
}

//------------------------------------------------------------------------------
// Convert a URI request (file:// or t3d://) to a file path relative to the game directory.
// Return true if the file exists.
bool FileRequestHandler::uriToFilePath(const CefString& uri)
{
   String filename = uri.ToString().c_str();

   if (filename.startsWith("t3d://") || filename.startsWith("file://"))
   {  // Strip "t3d://" or "file:/" off the front of the request.
      filename.erase(0, 6);
   }
   else
      return false;

   // Remove any remaining leading "/"'s.
   while (filename.startsWith("/"))
      filename.erase(0, 1);

   // Make a path relative to the game directory.
   const char *relPath = Platform::makeRelativePathName(filename.c_str(), NULL);
   if (relPath[0] && relPath[0] == '.' && relPath[1] && relPath[1] == '.')
   {  // If the path begins with "..", don't allow it.
      Con::errorf("Attempting to load file resource from outside the T3D game directory: %s", relPath);
      return false;
   }

   if (Torque::FS::IsFile(relPath))
   {
      _file_path = relPath;
      return true;
   }

   Con::errorf("File not found: %s", relPath);
   return false;
}

//------------------------------------------------------------------------------
