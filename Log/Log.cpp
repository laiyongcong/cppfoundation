#include "Log.h"
#include "Thread.h"
#include "RefelectionHelper.h"

namespace cppfd {
static const ReflectiveClass<LogConfig> g_refLogConfig = ReflectiveClass<LogConfig>()
                                                             .RefField(&LogConfig::FileCut, "FileCut")
                                                             .RefField(&LogConfig::FileName, "FileName")
                                                             .RefField(&LogConfig::KeepDays, "KeepDays")
                                                             .RefField(&LogConfig::LogLevel, "LogLevel")
                                                             .RefField(&LogConfig::MaxSize, "MaxSize")
                                                             .RefField(&LogConfig::Path, "Path")
                                                             .RefField(&LogConfig::ProcessName, "ProcessName");

const char* gLogDesc[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

const char* gLogFile[] = {"ERROR", "ERROR", "INFO", "INFO", "INFO"};

const char* GetLogDesc(ELogLevel logLevel) {
  if (logLevel < 0 || logLevel >= ELogLevel_Num) return "";
  return gLogDesc[logLevel];
}

const char* GetLogFilePrefix(ELogLevel logLevel) {
  if (logLevel < 0 || logLevel >= ELogLevel_Num) return "";
  return gLogFile[logLevel];
}

struct LogItem {
  ELogLevel mLogLevel;
  timeval mLogTime;
  String mLogMsg;
  String mThreadName;
  LogItem() : mLogLevel(ELogLevel_Info), mThreadName("UnknowThread") {}
};


class LogImp : public Thread {
 public:
  LogImp(const LogConfig& cfg) : mCfg(cfg) { 
    Utils::StringTrim(mCfg.FileName);
    Utils::StringTrim(mCfg.Path);
    std::replace(mCfg.Path.begin(), mCfg.Path.end(), '\\', '/');
    Utils::MkDirs(mCfg.Path);
    mCfg.MaxSize *= 1024 * 1024;
    if (!Utils::StringEndWith(mCfg.Path, "/")) mCfg.Path.append("/");
  }
  void Flush() { 
    LogItem logItem;
    while (mLogQueue.Dequeue(logItem)) {
      char szLogPrefix[1024], szFileName[1024];
      time_t tNow = logItem.mLogTime.tv_sec;
      tm tmNow;
      localtime_r(&tNow, &tmNow);
      safe_printf(szLogPrefix, sizeof(szLogPrefix), "%s %04d-%02d-%02d %02d:%02d:%02d.%03d %s-%d-%s", GetLogDesc(logItem.mLogLevel), tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
                  tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec, (int)(logItem.mLogTime.tv_usec / 1000), mCfg.ProcessName.c_str(), Utils::GetProcessID(), logItem.mThreadName.c_str());
      if (mCfg.FileCut == ELogCut_Day) {
        safe_printf(szFileName, sizeof(szFileName), "%s_%s_%04d%02d%02d%02d%02d%02d.log", mCfg.FileName.c_str(), GetLogFilePrefix(logItem.mLogLevel), tmNow.tm_year + 1900, tmNow.tm_mon + 1,
                    tmNow.tm_mday, 0, 0, 0);
      } else if (mCfg.FileCut == ELogCut_Hour) {
        safe_printf(szFileName, sizeof(szFileName), "%s_%s_%04d%02d%02d%02d%02d%02d.log", mCfg.FileName.c_str(), GetLogFilePrefix(logItem.mLogLevel), tmNow.tm_year + 1900, tmNow.tm_mon + 1,
                    tmNow.tm_mday, tmNow.tm_hour, 0, 0);
      } else {
        safe_printf(szFileName, sizeof(szFileName), "%s_%s_%04d%02d%02d%02d%02d%02d.log", mCfg.FileName.c_str(), GetLogFilePrefix(logItem.mLogLevel), tmNow.tm_year + 1900, tmNow.tm_mon + 1,
                    tmNow.tm_mday, tmNow.tm_hour, tmNow.tm_min, 0);
      }
      mStreamBuff[szFileName] << szLogPrefix << logItem.mLogMsg << std::endl;
      if (logItem.mLogLevel < ELogLevel_Warning) {
        if (mCfg.FileCut == ELogCut_Day) {
          safe_printf(szFileName, sizeof(szFileName), "%s_%s_%04d%02d%02d%02d%02d%02d.log", mCfg.FileName.c_str(), GetLogFilePrefix(ELogLevel_Warning), tmNow.tm_year + 1900, tmNow.tm_mon + 1,
                      tmNow.tm_mday, 0, 0, 0);
        } else if (mCfg.FileCut == ELogCut_Hour) {
          safe_printf(szFileName, sizeof(szFileName), "%s_%s_%04d%02d%02d%02d%02d%02d.log", mCfg.FileName.c_str(), GetLogFilePrefix(ELogLevel_Warning), tmNow.tm_year + 1900, tmNow.tm_mon + 1,
                      tmNow.tm_mday, tmNow.tm_hour, 0, 0);
        } else {
          safe_printf(szFileName, sizeof(szFileName), "%s_%s_%04d%02d%02d%02d%02d%02d.log", mCfg.FileName.c_str(), GetLogFilePrefix(ELogLevel_Warning), tmNow.tm_year + 1900, tmNow.tm_mon + 1,
                      tmNow.tm_mday, tmNow.tm_hour, tmNow.tm_min, 0);
        }
        mStreamBuff[szFileName] << szLogPrefix << logItem.mLogMsg << std::endl;
      }
      
    }
    for (auto& it : mStreamBuff) {
      String strFileName = it.first;
      if (strFileName.find(GetLogFilePrefix(ELogLevel_Warning)) != strFileName.npos)
        fprintf(stdout, "%s", it.second.str().c_str());
      const char* szFileName = (mCfg.Path + strFileName).c_str();
      FILE* fd = fopen(szFileName, "ab+");
      if (fd == nullptr) {
        printf("Log File:%s Open Error %s\n", szFileName, strerror(errno));
        continue;
      }

      fprintf(fd, "%s", it.second.str().c_str());
      fseek(fd, 0L, SEEK_END);
      uint64_t ufilesize = (uint64_t)ftell(fd);
      fclose(fd);
      fd = nullptr;

      if (ufilesize >= mCfg.MaxSize) {
        char newFileName[1024] = {0};
        uint64_t uNow = (uint64_t)time(nullptr);
        safe_printf(newFileName, sizeof(newFileName), "%s.%" PRIu64 ".log", szFileName, uNow);
        rename(szFileName, newFileName);
      }
    }
    mStreamBuff.clear();
  }

  void RemoveTimeoutFile() { 
    String strPattern = mCfg.Path + "*.log";
    std::vector<String> FileList;
    Utils::FindFiles(strPattern, false, &FileList);
    time_t tNow = time(nullptr);
    time_t tKeepSec = mCfg.KeepDays * 86400;
    for (size_t i = 0, isize = FileList.size(); i < isize; i++) {
      struct stat buf;
      const char* szFileName = FileList[i].c_str();
      int nRes = stat(szFileName, &buf);
      if (nRes != 0) {
        switch (errno) {
          case ENOENT:
            LOG_WARNING("File %s not found.\n", szFileName);
            break;
          case EINVAL:
            LOG_WARNING("Invalid parameter to _stat, file:%s.\n", szFileName);
            break;
          default:
            LOG_WARNING("Unexpected error in _stat, file:%s.\n", szFileName);
            break;
        }
        continue;
      }
      if (tNow - buf.st_mtime > tKeepSec) {
        remove(szFileName);
      }
    }
  }

  void WriteLog(const LogItem& logItem) { mLogQueue.Enqueue(logItem); }

 private:
  LogConfig mCfg;
  TMultiV1Queue<LogItem> mLogQueue;
  std::map<String, std::ostringstream> mStreamBuff;
};
LogImp* gLogInstance = nullptr;

bool Log::Init(const LogConfig& cfg) { 
  if (gLogInstance != nullptr) return true;
  gLogInstance = new LogImp(cfg);
  if (gLogInstance == nullptr) return false;
  gLogInstance->Start("Log");
  gLogInstance->Post([]() {
    uint64_t uCounter = 0;
    while (gLogInstance->IsRunning()) {
      Thread::Milisleep(10);
      uCounter++;
      if (uCounter % 10 == 0) {
        gLogInstance->Flush();
      }
      if (uCounter % 360000 == 0) {
        gLogInstance->RemoveTimeoutFile();
      }
    }
    gLogInstance->Flush();
  });
  return true; 
}

void Log::Destroy() {
  gLogInstance->Stop();
  SAFE_DELETE(gLogInstance); 
}

void Log::WriteLog(ELogLevel eLogLevel, const char* szFormat, ...) { 
  LogItem logItem;
  logItem.mLogLevel = eLogLevel;
  Utils::GetTimeOfDay(&logItem.mLogTime, nullptr);

  char szBuff[4096];
  va_list args;
  va_start(args, szFormat);
  vsnprintf(szBuff, sizeof(szBuff) - 1, szFormat, args);
  va_end(args);
  szBuff[sizeof(szBuff) - 1] = 0;

  logItem.mLogMsg = szBuff;
  Thread* pCurrThread = Thread::GetCurrentThread();
  if (pCurrThread) logItem.mThreadName = pCurrThread->GetName();

  if (gLogInstance) gLogInstance->WriteLog(logItem);
}

bool Log::IsValid() { return gLogInstance != nullptr; }

const char* Log::GetFileName(const char* szFileName) {
  if (szFileName == nullptr) return "";

  int nlen = (int)strlen(szFileName);

  for (int i = nlen - 1; i >= 0; i--) {
    if (szFileName[i] == '/' || szFileName[i] == '\\') {
      return (szFileName + i + 1);
    }
  }

  return szFileName;
}

const char* Log::GetFuncName(const char* szFuncName) {
  if (szFuncName == nullptr) return "";

  int nlen = (int)strlen(szFuncName);
  int nPos = 0;

  for (int i = 0; i < nlen - 2; i++) {
    if (szFuncName[i] == ':' && szFuncName[i + 1] == ':') {
      nPos = i + 2;
      i++;
    }

    if (szFuncName[i] == '(') {
      break;
    }
  }

  return szFuncName + nPos;
}

}