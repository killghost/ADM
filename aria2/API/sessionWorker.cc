#include "sessionWorker.h"
#include "sessionManager.h"
#include "common.h"

#include <algorithm>

napi_value AriaSessionWorker::createSession(std::string sesId, napi_ref callback) {
  napi_value resource_name;

  this->sesId = sesId;
  this->callback = callback;

  NAPI_CALL(env, napi_create_string_utf8(env, "SessionResource", NAPI_AUTO_LENGTH, &resource_name));

  NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name, 
    ExecuteSessionInit, CompleteSessionInit, this, &request));

  NAPI_CALL(env, napi_queue_async_work(env, request));

  return nullptr;
}

napi_value AriaSessionWorker::killAllSession(napi_ref callback) {
  napi_value resource_name;

  this->callback = callback;

  NAPI_CALL(env, napi_create_string_utf8(env, "SessionResource", NAPI_AUTO_LENGTH, &resource_name));

  NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name, 
    ExecuteKillAllSession, CompleteKillAllSession, this, &request));

  NAPI_CALL(env, napi_queue_async_work(env, request));

  return nullptr;
}

napi_value AriaSessionWorker::killSession(std::string sesId, napi_ref callback) {
  napi_value resource_name;

  this->sesId = sesId;
  this->callback = callback;

  NAPI_CALL(env, napi_create_string_utf8(env, "SessionResource", NAPI_AUTO_LENGTH, &resource_name));

  NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name, 
    ExecuteKillSession, CompleteKillSession, this, &request));

  NAPI_CALL(env, napi_queue_async_work(env, request));

  return nullptr;
}

napi_value AriaSessionWorker::pauseAllSession(napi_ref callback) {
  napi_value resource_name;

  this->callback = callback;

  NAPI_CALL(env, napi_create_string_utf8(env, "SessionResource", NAPI_AUTO_LENGTH, &resource_name));

  NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name, 
    ExecutePauseAllSession, CompletePauseAllSession, this, &request));

  NAPI_CALL(env, napi_queue_async_work(env, request));

  return nullptr;
}

napi_value AriaSessionWorker::pauseSession(std::string sesId, napi_ref callback) {
  napi_value resource_name;

  this->sesId = sesId;
  this->callback = callback;

  NAPI_CALL(env, napi_create_string_utf8(env, "SessionResource", NAPI_AUTO_LENGTH, &resource_name));

  NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name, 
    ExecutePauseSession, CompletePauseSession, this, &request));

  NAPI_CALL(env, napi_queue_async_work(env, request));

  return nullptr;
}

void ExecuteSessionInit(napi_env env, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);

  aria2::SessionConfig config;

  config.downloadEventCallback = downloadEventCallback;
  config.keepRunning = true;
  worker->session = aria2::sessionNew(aria2::KeyVals(), config);

  SessionManager::getInstance()->addSession(std::pair<std::string, aria2::Session*>(worker->sesId, worker->session));
  SessionManager::getInstance()->addSessionRunWorker(std::thread([=]() {
    aria2::Session* session = SessionManager::getInstance()->getSession(worker->sesId);

    for (;;) {
      if(!session) {
        std::cerr << "testasdasdsada";
        break;
      }
      aria2::run(session, aria2::RUN_ONCE);
    } 
  }));
  
}

void ExecuteKillAllSession(napi_env env, void *data) {
  std::map<std::string, aria2::Session*>::iterator it;
  std::map<std::string, aria2::Session*> sessionMap = SessionManager::getInstance()->getSessionMap();

  for(it = sessionMap.begin(); it != sessionMap.end(); it++){
    aria2::sessionFinal(it->second);
    aria2::shutdown(it->second);
  }

  if(sessionMap.size() != 0) {
    SessionManager::getInstance()->clearAllSession();
  }

  std::for_each(SessionManager::getInstance()->getRunWorker()->begin(), SessionManager::getInstance()->getRunWorker()->end(), [](std::thread &t) 
  {
    t.join();
  });
}

void ExecuteKillSession(napi_env env, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);

  std::map<std::string, aria2::Session*>::iterator it;
  std::map<std::string, aria2::Session*> sessionMap = SessionManager::getInstance()->getSessionMap();

  it = sessionMap.find(worker->sesId);
  aria2::Session* session = it->second;

  aria2::shutdown(session);

  SessionManager::getInstance()->clearSession(it->first);
  session = nullptr;
}

void ExecutePauseAllSession(napi_env env, void *data) {
  std::map<std::string, aria2::Session*> sessionMap = SessionManager::getInstance()->getSessionMap();
  
  for(auto ses: sessionMap) {
    std::vector<aria2::A2Gid> allGids = aria2::getActiveDownload(ses.second);

    for(const auto& gid : allGids){
      aria2::pauseDownload(ses.second, gid);
      std::cout << aria2::gidToHex(gid) << std::endl;
    }
  }
}

void ExecutePauseSession(napi_env env, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);
  aria2::Session* session = SessionManager::getInstance()->getSession(worker->sesId);

  std::vector<aria2::A2Gid> allGids = aria2::getActiveDownload(session);
  
  for(const auto& gid : allGids){
    aria2::pauseDownload(session, gid);
    std::cout << aria2::gidToHex(gid) << std::endl;
  }
}

void CompleteSessionInit(napi_env env, napi_status status, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);

  napi_value argv[2];

  NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &argv[0]));
  NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, worker->sesId.c_str(), worker->sesId.length() , &argv[1]));

  napi_value localCallback;
  NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, worker->callback, &localCallback));

  napi_value global;
  NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));

  napi_value result;
  NAPI_CALL_RETURN_VOID(env, napi_call_function(env, global, localCallback, 2, argv, &result));

  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, worker->callback));
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, worker->request));

  delete worker;
}

void CompleteKillAllSession(napi_env env, napi_status status, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);

  napi_value argv[2];

  std::string errorMsg = "Kill All Session Error";

  if(status != napi_ok) {
    NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, errorMsg.c_str(), errorMsg.length(), &argv[0]));
    NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, 0, &argv[1]));
  }
  else {
    NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &argv[0]));
    NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, 1, &argv[1]));
  }

  napi_value localCallback;
  NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, worker->callback, &localCallback));

  napi_value global;
  NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));

  napi_value result;
  NAPI_CALL_RETURN_VOID(env, napi_call_function(env, global, localCallback, 2, argv, &result));

  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, worker->callback));
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, worker->request));

  delete worker;
}

void CompleteKillSession(napi_env env, napi_status status, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);
  
  napi_value argv[2];

  NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &argv[0]));
  NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, worker->sesId.c_str(), worker->sesId.length() , &argv[1]));

  napi_value localCallback;
  NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, worker->callback, &localCallback));

  napi_value global;
  NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));

  napi_value result;
  NAPI_CALL_RETURN_VOID(env, napi_call_function(env, global, localCallback, 2, argv, &result));

  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, worker->callback));
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, worker->request));

  delete worker;
}

void CompletePauseAllSession(napi_env env, napi_status status, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);
  
  napi_value argv[2];

  NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &argv[0]));
  NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, 1, &argv[1]));

  napi_value localCallback;
  NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, worker->callback, &localCallback));

  napi_value global;
  NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));

  napi_value result;
  NAPI_CALL_RETURN_VOID(env, napi_call_function(env, global, localCallback, 2, argv, &result));

  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, worker->callback));
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, worker->request));

  delete worker;
}

void CompletePauseSession(napi_env env, napi_status status, void *data) {
  AriaSessionWorker* worker = static_cast<AriaSessionWorker *>(data);
  
  napi_value argv[2];

  NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &argv[0]));
  NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, worker->sesId.c_str(), worker->sesId.length() , &argv[1]));

  napi_value localCallback;
  NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, worker->callback, &localCallback));

  napi_value global;
  NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));

  napi_value result;
  NAPI_CALL_RETURN_VOID(env, napi_call_function(env, global, localCallback, 2, argv, &result));

  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, worker->callback));
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, worker->request));

  delete worker;
}

int downloadEventCallback(aria2::Session* session, aria2::DownloadEvent event,
                          aria2::A2Gid gid, void* userData)
{
  switch (event) {
    case aria2::EVENT_ON_DOWNLOAD_COMPLETE:
      std::cerr << "COMPLETE";
      break;
    case aria2::EVENT_ON_DOWNLOAD_ERROR:
      std::cerr << "ERROR";
      break;
    default:
      return 0;
  }

  std::cerr << " [" << aria2::gidToHex(gid) << "] ";
  aria2::DownloadHandle* dh = aria2::getDownloadHandle(session, gid);

  if (!dh)
    return 0;
  if (dh->getNumFiles() > 0) {
    aria2::FileData f = dh->getFile(1);

    // Path may be empty if the file name has not been determined yet.
    if (f.path.empty()) {
      if (!f.uris.empty()) {
        std::cerr << f.uris[0].uri;
      }
    }
    else {
      std::cerr << f.path;
    }
  }

  aria2::deleteDownloadHandle(dh);
  std::cerr << std::endl;
  return 0;
}