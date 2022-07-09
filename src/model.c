#include "string.h"

// Each model can have 3 alternative names
struct CanonModels {
	char *alts[3];
};

struct CanonModels models[] = {
				{{ "EOS 100D", "Rebel SL1", "Kiss X7" }},
				{{ "EOS 200D", "Rebel SL2", "Kiss X9" }},
				{{ "EOS 500D", "Rebel T1i", "Kiss X3" }},
				{{ "EOS 550D", "Rebel T2i", "Kiss X4" }},
				{{ "EOS 600D", "Rebel T3i", "Kiss X5" }},
				{{ "EOS 650D", "Rebel T4i", "Kiss X6i" }},
				{{ "EOS 700D", "Rebel T5i", "Kiss X7i" }},
				{{ "EOS 750D", "Rebel T6i", "Kiss X8i" }},
				{{ "EOS 760D", "Rebel T6s", "8000D" }},
				{{ "EOS 800D", "Rebel T7i", "Kiss X9i" }},
				{{ "EOS 1100D", "Rebel T3", "Kiss X50" }},
				{{ "EOS 1200D", "Rebel T5", "Kiss X70" }},
				{{ "EOS 1300D", "Rebel T6", "Kiss X80" }},
				{{ "EOS 2000D", "Rebel T7", "Kiss X90" }},
				{{ "EOS 4000D", "Rebel T100", "3000D" }},
				{{ "EOS 50D", 0, 0 }},
				{{ "EOS 60D", 0, 0 }},
				{{ "EOS 60Da", 0, 0 }},
				{{ "EOS 70D", 0, 0 }},
				{{ "EOS 80D", 0, 0 }},
				{{ "EOS 6D", 0, 0 }},
				{{ "EOS 6D Mark II", 0, 0 }},
				{{ "EOS 7D", 0, 0 }},
				{{ "EOS 7D Mark II", 0, 0 }},
				{{ "EOS 77D", "9000D", 0 }},
				{{ "EOS 5D Mark II", 0, 0 }},
				{{ "EOS 5D Mark III", 0, 0 }},
				{{ "EOS 5D Mark IV", 0, 0 }},
				{{ "EOS 5Ds", 0, 0 }},
				{{ "EOS 5Ds R", 0, 0 }},
				{{ "EOS-1D Mark IV", 0, 0 }},
				{{ "EOS-1D C", 0, 0 }},
				{{ "EOS-1D X", 0, 0 }},
				{{ "EOS-1D X Mark II", 0, 0 }} };

int model_get(char name[])
{
	for (int m = 0; m < (int)(sizeof(models) / sizeof(models[0])); m++) {
		for (int a = 0; a < 3; a++) {
			// Continue if no alt name
			if (models[m].alts[a] == 0) {
				continue;
			}

			// Use strstr to find whether "models[m].alts[a]" is
			// inside of "name". Basically String.includes(...)
			if (strstr(name, models[m].alts[a])) {				
				return m;
			}
		}
	}

	return -1;
}
