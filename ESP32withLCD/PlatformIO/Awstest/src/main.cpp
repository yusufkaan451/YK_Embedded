#include <aws/core/Aws.h>
...

Aws::SDKOptions options;
Aws::InitAPI(options);

//use the sdk

Aws::ShutdownAPI(options);
