#ifndef _LOCK_H
#define _LOCK_H
class CLock
{
public:
  CLock()
  {
    InitializeCriticalSection(&m_cs);
  }
  ~CLock()
  {
    DeleteCriticalSection(&m_cs);
  }
  void Lock()
  {
    EnterCriticalSection(&m_cs);
  }
  void Unlock()
  {
    LeaveCriticalSection(&m_cs);
  }

private:
  CRITICAL_SECTION m_cs;
};
#endif