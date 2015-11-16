#ifndef ComponentBase_h
#define ComponentBase_h

#include <iostream>
#include <cstring>
namespace elx
{
  class ComponentBase {
  public:
    virtual int ConnectFrom(const char *, ComponentBase*) { return 0; }; //= 0;
  protected:
    virtual ~ComponentBase(){};
  };
} // end namespace elx
#endif // #define ComponentBase_h