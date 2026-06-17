# 📍 ONDE ESTÃO SENDO SALVOS OS OFFSETS?

## 🔄 Fluxo Completo de Dados

```
┌─────────────────────────────────────────────────────────────────┐
│                    ENTRY.CPP (main)                             │
│  g_Updater->Init()  →  g_Updater->Scan()  →  g_Updater->Release() │
└────────────────┬────────────────────────┬──────────────────────────┘
				 │                        │
				 ▼                        ▼
		┌────────────────┐      ┌──────────────────┐
		│ SetupPatterns()│      │  Updater::Scan() │
		└────────┬───────┘      └────────┬─────────┘
				 │                       │
				 ▼                       ▼
		┌─────────────────────────────────────────┐
		│    AUTO_OFFSET Macros (Updater.h)      │
		│  Cria objetos AutoOffset para cada     │
		│  offset e VINCULA UMA REFERÊNCIA       │
		│                                         │
		│ SetReference(&Offsets::World::Camera)  │
		└──────┬────────────────────────────────┘
			   │
			   ▼
		┌──────────────────────┐
		│  AutoOffset::Scan()  │
		│  Procura o padrão    │
		│  Salva em m_Offset   │
		└──────┬───────────────┘
			   │
			   ▼ (em Release())
		┌──────────────────────────────┐
		│  UpdateReference()           │
		│  *m_Reference = m_Offset;    │
		│  Escreve o offset encontrado │
		│  na variável global          │
		└──────┬───────────────────────┘
			   │
			   ▼
		┌──────────────────────────────────────────┐
		│  OFFSETS.H - Variáveis Globais Estáticas │
		│                                          │
		│  namespace Offsets {                     │
		│    namespace World {                     │
		│      inline INT64 Camera = VALOR;  ◄────── SALVO AQUI!
		│    }                                     │
		│    namespace Weapon {                    │
		│      inline INT64 WeaponIndex = VALOR;  │
		│    }                                     │
		│  }                                       │
		└──────────────────────────────────────────┘
```

---

## 📦 LOCAL DE ARMAZENAMENTO (Offsets.h)

Os offsets são salvos como **variáveis globais estáticas** (inline) em namespaces:

```cpp
// OFFSETS.H - O local real onde os dados são salvos

namespace Offsets {
	// Modbase Offsets
	namespace Modbase {
		inline INT64 World = 0x0;      // Salvará aqui!
		inline INT64 Network = 0x0;    // Salvará aqui!
		inline INT64 Tick = 0x0;       // Salvará aqui!
	}

	// World Offsets
	namespace World {
		inline INT64 BulletList = 0x0;     // Salvará aqui!
		inline INT64 NearEntList = 0x0;    // Salvará aqui!
		inline INT64 FarEntList = 0x0;     // Salvará aqui!
		inline INT64 Camera = 0x0;         // Salvará aqui!
		inline INT64 LocalPlayer = 0x0;    // Salvará aqui!
		inline INT64 LocalOffset = 0x0;    // Salvará aqui!
	}

	// Weapon Offsets
	namespace Weapon {
		inline INT64 WeaponIndex = 0x0;        // Salvará aqui!
		inline INT64 WeaponInfoTable = 0x0;    // Salvará aqui!
		inline INT64 MuzzleCount = 0x0;        // Salvará aqui!
		inline INT64 WeaponInfoSize = 0x0;     // Salvará aqui!
	}

	// ... E muitas outras estruturas
}
```

---

## 🔗 Fluxo Passo a Passo

### 1️⃣ **SetupPatterns() - Registra os Offsets**
Arquivo: `Updater.cpp`, linhas ~195+

```cpp
void Updater::SetupWorldPatterns() {
	// Esta macro cria um AutoOffset E vincula uma referência
	AUTO_OFFSET(World, Camera, 
		"\\x4C\\x8B\\x83\\x00\\x00\\x00\\x00\\x4C\\x8B\\x11\\x48\\x89\\x70\\x08",
		"xxx????xxxxxxx",
		".text",
		ScanType::MovReg,
		0
	);
	// Internamente:
	// - Cria um AutoOffset
	// - SetReference(&Offsets::World::Camera)  ◄── VINCULA A REFERÊNCIA
}
```

### 2️⃣ **AUTO_OFFSET Macro (Updater.h, linhas ~83-90)**
```cpp
#define AUTO_OFFSET(Klass, Name, Pattern, Mask, Section, Type, Offset)\
	AutoOffset Name;                                    // Cria objeto\
	Name.SetPattern((PBYTE)Pattern);                    // Configura\
	Name.SetMask(Mask);                                 // Configura\
	Name.SetSection(Section);                           // Configura\
	Name.SetReference(&Offsets::Klass::Name);           // VINCULA! ◄──\
	Name.SetOffset(Offset);                             // Configura\
	m_Scans[#Klass##"::"#Name] = Name;                  // Armazena
```

Aqui está o truque! `Name.SetReference(&Offsets::Klass::Name)` armazena o **endereço da variável global** em `m_Reference`.

### 3️⃣ **Updater::Scan() - Procura os Padrões**
Arquivo: `Updater.cpp`, linhas ~365+

```cpp
bool Updater::Scan() {
	for (auto& Data : m_Scans) {
		Data.second.Scan(m_Module, m_Allocated);  // Procura padrão
		// m_Offset é preenchido aqui ▲
	}
	return true;
}
```

### 4️⃣ **Updater::Release() - Salva os Offsets**
Arquivo: `Updater.cpp`, linhas ~380+

```cpp
bool Updater::Release() {
	for (auto& Data : m_Scans) {
		if (!Data.second.UpdateReference()) {  // Aqui salva!
			// UpdateReference() faz: *m_Reference = m_Offset;
			// Ou seja: *(&Offsets::World::Camera) = offset_encontrado;
			printf("[UPDATER] Failed to get offset: %s\n", Data.first.c_str());
		}
		printf("[UPDATER] %-36s -> 0x%p\n", Data.first.c_str(), 
			   Data.second.GetOffset());
	}
	m_Scans.clear();
	(void)DeallocateModule();
	return Result;
}
```

---

## 🗂️ Resumo dos Locais

| Componente | Localização | Tipo |
|-----------|------------|------|
| **Variáveis de Offset** | `Offsets.h` (linhas 8-95) | `inline INT64` em namespaces |
| **Referências** | `m_Reference` em `AutoOffset` | Ponteiro para `Offsets::*` |
| **Valores Encontrados** | `m_Offset` em `AutoOffset` | `INT64` temporário |
| **Transferência** | `UpdateReference()` | `*m_Reference = m_Offset;` |
| **Descartes** | `m_Scans.clear()` | Objetos `AutoOffset` deletados |

---

## 📊 Exemplo Prático: World::Camera

```cpp
// ESTADO INICIAL (Offsets.h)
namespace Offsets {
	namespace World {
		inline INT64 Camera = 0x0;  // ← Começa com 0
	}
}

// DURANTE SetupWorldPatterns()
AUTO_OFFSET(World, Camera, ...);
// Isso internamente: SetReference(&Offsets::World::Camera);
// Guarda o endereço: m_Reference = 0x7ff12345 (endereço de Offsets::World::Camera)

// DURANTE Scan()
// PatternScan encontra em 0x001A5D30
// m_Offset = 0x001A5D30

// DURANTE Release() - UpdateReference()
*m_Reference = m_Offset;
// Equivalente a: *(0x7ff12345) = 0x001A5D30;
// Ou seja: Offsets::World::Camera = 0x001A5D30;

// ESTADO FINAL (Offsets.h)
namespace Offsets {
	namespace World {
		inline INT64 Camera = 0x001A5D30;  // ← Salvo aqui!
	}
}
```

---

## ⚡ Como Acessar os Offsets Depois

Após `Release()`, você pode usar os offsets em qualquer lugar:

```cpp
// Em qualquer arquivo
#include "Offsets.h"

INT64 CameraOffset = Offsets::World::Camera;
INT64 WeaponIndex = Offsets::Weapon::WeaponIndex;
INT64 LocalPlayer = Offsets::World::LocalPlayer;

// Se precisa de pointer
INT64* CameraPtr = &Offsets::World::Camera;
```

---

## 🎯 Resumo Final

✅ **Os offsets são salvos em:** `Offsets.h` - Variáveis globais inline  
✅ **Mecanismo:** Ponteiros para referência (`m_Reference`)  
✅ **Salvamento:** Função `UpdateReference()` escreve o valor  
✅ **Acesso:** Qualquer arquivo que include `Offsets.h`  
✅ **Tipo:** `inline INT64` em namespaces aninhados  
✅ **Limpeza:** `m_Scans.clear()` após `Release()`  

---

## 🔍 Para Debugar Offsets Salvos

Adicione isto em `Release()`:

```cpp
#if _DEBUG
printf("\n=== OFFSETS SALVOS ===\n");
printf("Offsets::World::Camera = 0x%llx\n", Offsets::World::Camera);
printf("Offsets::World::LocalPlayer = 0x%llx\n", Offsets::World::LocalPlayer);
printf("Offsets::Weapon::WeaponIndex = 0x%llx\n", Offsets::Weapon::WeaponIndex);
printf("===================\n\n");
#endif
```
