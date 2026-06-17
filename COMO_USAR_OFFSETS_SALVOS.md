# 🚀 EXEMPLO: COMO USAR OS OFFSETS SALVOS

## Uso Após Release()

```cpp
// ARQUIVO: main.cpp ou qualquer outro

#include "Offsets.h"  // Inclua este arquivo!
#include <stdio.h>

int main() {
	// ... Seu código de inicialização ...

	// Inicializa o updater
	if (!g_Updater->Init()) {
		printf("Falha ao inicializar!\n");
		return 0;
	}

	// Escaneia os padrões
	if (!g_Updater->Scan()) {
		printf("Falha ao escanear!\n");
		return 0;
	}

	// Salva os offsets
	if (!g_Updater->Release()) {
		printf("Falha ao liberar/salvar!\n");
		return 0;
	}

	// ✅ AGORA OS OFFSETS ESTÃO SALVOS E PRONTOS PARA USO!
	printf("\n=== OFFSETS SALVOS COM SUCESSO ===\n");
	printf("Offsets::World::Camera = 0x%llx\n", Offsets::World::Camera);
	printf("Offsets::World::LocalPlayer = 0x%llx\n", Offsets::World::LocalPlayer);
	printf("Offsets::Weapon::WeaponIndex = 0x%llx\n", Offsets::Weapon::WeaponIndex);
	printf("===================================\n\n");

	return 1;
}
```

---

## Exemplo Prático: Usando os Offsets para Ler Estruturas

```cpp
#include "Offsets.h"
#include <cstdint>

// Suponha que você tenha uma classe World
class World {
public:
	// ... outros membros ...
};

// Função para obter a instância de World
World* GetWorldInstance(uint64_t moduleBase) {
	// Offsets::Modbase::World contém o offset da World
	uint64_t worldPtrAddress = moduleBase + Offsets::Modbase::World;

	// Lê o ponteiro da memória
	World** worldPtr = (World**)(worldPtrAddress);
	return *worldPtr;
}

// Função para obter a câmera
void* GetCamera(uint64_t moduleBase) {
	World* world = GetWorldInstance(moduleBase);
	if (!world) return nullptr;

	// Offsets::World::Camera contém o offset dentro de World
	void* camera = (void*)((uint64_t)world + Offsets::World::Camera);
	return camera;
}

// Função para obter o jogador local
void* GetLocalPlayer(uint64_t moduleBase) {
	World* world = GetWorldInstance(moduleBase);
	if (!world) return nullptr;

	// Offsets::World::LocalPlayer contém o offset
	void* player = (void*)((uint64_t)world + Offsets::World::LocalPlayer);
	return player;
}

// Exemplo de uso
int main() {
	// ... após Release() ...

	uint64_t gameModule = 0x7FF000000;  // Exemplo

	// Obtém a câmera
	void* camera = GetCamera(gameModule);
	printf("Camera: %p\n", camera);

	// Obtém o jogador
	void* player = GetLocalPlayer(gameModule);
	printf("Player: %p\n", player);

	return 0;
}
```

---

## Exemplo: Estrutura de Dados com Offsets

```cpp
#include "Offsets.h"

// Estrutura do Jogador
struct Player {
	uint8_t padding[Offsets::DayZPlayer::Skeleton];
	class Skeleton* skeleton;

	uint8_t padding2[Offsets::DayZPlayer::NetworkID - Offsets::DayZPlayer::Skeleton - 8];
	uint32_t networkID;

	uint8_t padding3[Offsets::DayZPlayer::Inventory - Offsets::DayZPlayer::NetworkID - 4];
	void* inventory;
};

// Estrutura da Arma
struct Weapon {
	uint8_t padding1[Offsets::Weapon::WeaponIndex];
	uint32_t weaponIndex;

	uint8_t padding2[Offsets::Weapon::WeaponInfoTable - Offsets::Weapon::WeaponIndex - 4];
	void* weaponInfoTable;

	uint8_t padding3[Offsets::Weapon::MuzzleCount - Offsets::Weapon::WeaponInfoTable - 8];
	uint32_t muzzleCount;
};

// Uso
int main() {
	// ... após Release() ...

	Player* player = (Player*)(playerAddress);

	// Acessa skeleton através do offset
	class Skeleton* skeleton = player->skeleton;

	// Acessa networkID através do offset
	uint32_t id = player->networkID;

	printf("Player ID: %u\n", id);
	printf("Skeleton: %p\n", skeleton);

	return 0;
}
```

---

## Exemplo: Verificação de Offsets

```cpp
#include "Offsets.h"
#include <stdio.h>

bool VerifyOffsets() {
	printf("Verificando offsets salvos...\n");

	// Verifica se todos os offsets foram encontrados
	bool allValid = true;

	// Modbase
	if (Offsets::Modbase::World == 0) {
		printf("❌ Modbase::World não foi encontrado!\n");
		allValid = false;
	} else {
		printf("✅ Modbase::World = 0x%llx\n", Offsets::Modbase::World);
	}

	// World
	if (Offsets::World::Camera == 0) {
		printf("❌ World::Camera não foi encontrado!\n");
		allValid = false;
	} else {
		printf("✅ World::Camera = 0x%llx\n", Offsets::World::Camera);
	}

	if (Offsets::World::LocalPlayer == 0) {
		printf("❌ World::LocalPlayer não foi encontrado!\n");
		allValid = false;
	} else {
		printf("✅ World::LocalPlayer = 0x%llx\n", Offsets::World::LocalPlayer);
	}

	// Weapon
	if (Offsets::Weapon::WeaponIndex == 0) {
		printf("❌ Weapon::WeaponIndex não foi encontrado!\n");
		allValid = false;
	} else {
		printf("✅ Weapon::WeaponIndex = 0x%llx\n", Offsets::Weapon::WeaponIndex);
	}

	// DayZPlayer
	if (Offsets::DayZPlayer::Skeleton == 0) {
		printf("❌ DayZPlayer::Skeleton não foi encontrado!\n");
		allValid = false;
	} else {
		printf("✅ DayZPlayer::Skeleton = 0x%llx\n", Offsets::DayZPlayer::Skeleton);
	}

	if (allValid) {
		printf("\n✅ Todos os offsets foram carregados com sucesso!\n");
	} else {
		printf("\n❌ Alguns offsets não foram encontrados!\n");
	}

	return allValid;
}

int main() {
	// ... após Release() ...

	if (!VerifyOffsets()) {
		printf("Erro: Nem todos os offsets foram encontrados.\n");
		printf("Verifique os padrões em SetupPatterns().\n");
		return 0;
	}

	// Continuar com o programa...
	printf("\nContinuando com offsets válidos...\n");
	return 1;
}
```

---

## Exemplo: Salvar/Carregar Offsets de Arquivo

```cpp
#include "Offsets.h"
#include <fstream>
#include <stdio.h>

// Salvar offsets em arquivo
bool SaveOffsetsToFile(const char* filename) {
	std::ofstream file(filename, std::ios::binary);

	if (!file.is_open()) {
		printf("Erro ao abrir arquivo para escrita: %s\n", filename);
		return false;
	}

	// Salva um offset por linha em formato texto
	fprintf(stderr, "World::Camera = 0x%llx\n", Offsets::World::Camera);
	fprintf(stderr, "World::LocalPlayer = 0x%llx\n", Offsets::World::LocalPlayer);
	fprintf(stderr, "Weapon::WeaponIndex = 0x%llx\n", Offsets::Weapon::WeaponIndex);

	// Ou salvar em binário:
	// INT64 camera = Offsets::World::Camera;
	// file.write((char*)&camera, sizeof(INT64));

	file.close();
	printf("✅ Offsets salvos em: %s\n", filename);
	return true;
}

// Carregar offsets de arquivo (simulado)
bool LoadOffsetsFromFile(const char* filename) {
	std::ifstream file(filename, std::ios::binary);

	if (!file.is_open()) {
		printf("Erro ao abrir arquivo para leitura: %s\n", filename);
		return false;
	}

	// Lê os offsets...
	// INT64 camera;
	// file.read((char*)&camera, sizeof(INT64));
	// Offsets::World::Camera = camera;

	file.close();
	printf("✅ Offsets carregados de: %s\n", filename);
	return true;
}

int main() {
	// ... após Release() ...

	// Salvar offsets
	SaveOffsetsToFile("offsets.bin");

	// Em outra execução, você pode recarregar
	// LoadOffsetsFromFile("offsets.bin");

	return 0;
}
```

---

## Checklist de Uso

✅ **Incluir Header:**
```cpp
#include "Offsets.h"
```

✅ **Executar Updater:**
```cpp
g_Updater->Init();
g_Updater->Scan();
g_Updater->Release();
```

✅ **Verificar se foi salvo:**
```cpp
if (Offsets::World::Camera != 0) {
	// Offset foi encontrado e salvo!
}
```

✅ **Usar os offsets:**
```cpp
void* ptr = (void*)(baseAddress + Offsets::World::Camera);
```

✅ **Acessar qualquer offset:**
```cpp
Offsets::World::Camera
Offsets::World::LocalPlayer
Offsets::Weapon::WeaponIndex
Offsets::DayZPlayer::Skeleton
// ... etc
```

---

## ⚠️ Alertas Comuns

❌ **NÃO FAIRE:**
```cpp
// Usar antes de Release()
int offset = Offsets::World::Camera;  // Ainda será 0!
```

✅ **CORRETO:**
```cpp
// Após Release()
if (Offsets::World::Camera != 0) {
	int offset = Offsets::World::Camera;  // Agora tem valor
}
```

❌ **NÃO FAIRE:**
```cpp
// Esquecer de incluir Offsets.h
printf("%llx", Offsets::World::Camera);  // Erro de compilação!
```

✅ **CORRETO:**
```cpp
// Incluir o header
#include "Offsets.h"
printf("%llx", Offsets::World::Camera);  // OK
```
