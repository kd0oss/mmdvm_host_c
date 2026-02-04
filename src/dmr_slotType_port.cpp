#include <cstring>
#include "dmr_slotType_port.h"

// MMDVMHost headers vendored under third_party/mmdvmhost
#include "DMRDefines.h"
#include "DMRSlotType.h"

int dmr_get_color_code(const uint8_t* header33, size_t len, uint8_t* cc)
{
  if (!header33 || len < 33 || !cc)
    return 0;

  CDMRSlotType st;
  st.putData(header33);

  *cc = st.getColorCode();
  return 1;
}

int dmr_get_data_type(const uint8_t* header33, size_t len, uint8_t* dt)
{
  if (!header33 || len < 33 || !dt)
    return 0;

  CDMRSlotType st;
  st.putData(header33);

  *dt = st.getDataType();
  return 1;
}
