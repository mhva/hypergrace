#ifndef UTIL_SHARED_HH_
#define UTIL_SHARED_HH_

#define HG_DECLARE_PRIVATE class Private; Private *d;
#define HG_DECLARE_CUSTOM_PRIVATE(cls) class cls; cls *d;
#define HG_DECLARE_SHARED_PRIVATE class Private; std::shared_ptr<Private> d;
#define HG_DECLARE_CUSTOM_SHARED_PRIVATE(cls) class cls; cls *d;

#include <memory>

#endif /* UTIL_SHARED_HH_ */
