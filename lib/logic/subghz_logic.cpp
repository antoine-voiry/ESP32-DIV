#include "subghz_logic.h"

int computeCC1101Status(bool chipDetected) {
    return chipDetected ? 0 : 1;
}
