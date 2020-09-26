

#include "hsutil.h"

bool CHsUtil::checkPhone(HCSTRR phone) {
    return phone.length() > 4 && phone.length() < 20;
}