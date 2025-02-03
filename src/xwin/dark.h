#include <windows.h>
#include <Uxtheme.h>

enum mode
{
   Default,
   AllowDark,
   ForceDark,
   ForceLight,
   Max
};
typedef enum mode PreferredAppMode;

typedef PreferredAppMode (*fnSetPreferredAppMode)(PreferredAppMode appMode);

BOOL regCheckDark(void);
void setAppModeDark(void);
