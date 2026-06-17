#include "Framework.h"
#include <fstream>
#include <sstream>

INT64 AutoOffset::ResolveMovCs(UINT64 Module, UINT64 Instruction) {

	int Relative = *(int*)(Instruction + 3);

	// instrução mov rax, [rel address] é: 0x48 0x8B 0x05 + 4 bytes de offset
	// a próxima instrução começa em Instruction + 7
	// o endereço absoluto é: (próxima_instrução + deslocamento_relativo)
	UINT64 NextInstruction = Instruction + 7;

	return (NextInstruction + Relative) - Module;
}

INT64 AutoOffset::ResolveCmpCs(UINT64 Module, UINT64 Instruction) {

	int Relative = *(int*)(Instruction + 2);

	// instrução cmp [rel address], ... é: 0x3D ou similar + 4 bytes de offset
	// a próxima instrução começa em Instruction + 7
	UINT64 NextInstruction = Instruction + 7;

	return (NextInstruction + Relative) - Module;
}

INT64 AutoOffset::ResolveMovRegXmm(UINT64 Module, UINT64 Instruction) {
	return *(int*)(Instruction + 4);
}

INT64 AutoOffset::ResolveMovRegXmmLrg(UINT64 Module, UINT64 Instruction) {
	return *(int*)(Instruction + 5);
}

INT64 AutoOffset::ResolveMovRegXmmLrgByte(UINT64 Module, UINT64 Instruction) {
	return *(BYTE*)(Instruction + 5);
}

INT64 AutoOffset::ResolveMovRegXmmByte(UINT64 Module, UINT64 Instruction) {
	return *(BYTE*)(Instruction + 4);
}

INT64 AutoOffset::ResolveMovReg(UINT64 Module, UINT64 Instruction) {
	return *(int*)(Instruction + 3);
}

INT64 AutoOffset::ResolveMovRegByte(UINT64 Module, UINT64 Instruction) {
	return *(BYTE*)(Instruction + 3);
}

INT64 AutoOffset::ResolveMovRegSml(UINT64 Module, UINT64 Instruction) {
	return *(int*)(Instruction + 2);
}

INT64 AutoOffset::ResolveMovRegByteSml(UINT64 Module, UINT64 Instruction) {
	return *(BYTE*)(Instruction + 2);
}

INT64 AutoOffset::ResolveTraceMovReg(UINT64 Module, UINT64 Instruction) {
	
	/* we save this cause we do the pattern scan on the call - and then the offset inside the call routine */
	Instruction -= m_InstructionOffset;

	auto JmpRva = *(int*)(Instruction + 1);

	Instruction += 5 + JmpRva;

	Instruction += m_InstructionOffset;

	return *(int*)(Instruction + 3);
}

INT64 AutoOffset::ResolveTraceMovRegByte(UINT64 Module, UINT64 Instruction) {
	
	/* we save this cause we do the pattern scan on the call - and then the offset inside the call routine */
	Instruction -= m_InstructionOffset;

	auto JmpRva = *(int*)(Instruction + 1);

	Instruction += 5 + JmpRva;

	Instruction += m_InstructionOffset;

	return *(BYTE*)(Instruction + 3);
}

bool AutoOffset::ResolveOffset(UINT64 Module, UINT64 Instruction) {

	Instruction += m_InstructionOffset;

	switch (m_Type) {

	case ScanType::MovReg:			m_Offset = ResolveMovReg(Module, Instruction);				break;
	case ScanType::MovRegByte:		m_Offset = ResolveMovRegByte(Module, Instruction);			break;
	case ScanType::MovRegXmm:		m_Offset = ResolveMovRegXmm(Module, Instruction);			break;
	case ScanType::MovRegXmmLrg:	m_Offset = ResolveMovRegXmmLrg(Module, Instruction);		break;
	case ScanType::MovRegXmmByte:	m_Offset = ResolveMovRegXmmByte(Module, Instruction);		break;
	case ScanType::MovRegXmmLrgByte:m_Offset = ResolveMovRegXmmLrgByte(Module, Instruction);	break;
	case ScanType::MovRegSml:		m_Offset = ResolveMovRegSml(Module, Instruction);			break;
	case ScanType::MovRegByteSml:	m_Offset = ResolveMovRegByteSml(Module, Instruction);		break;
	case ScanType::TraceMovReg:		m_Offset = ResolveTraceMovReg(Module, Instruction);			break;
	case ScanType::TraceMovRegByte: m_Offset = ResolveTraceMovRegByte(Module, Instruction);		break;
	case ScanType::MovCs:			m_Offset = ResolveMovCs(Module, Instruction);				break;
	case ScanType::CmpCs:			m_Offset = ResolveCmpCs(Module, Instruction);				break;

	}

	m_Offset += m_LastOffset;

	return m_Offset;
}

bool AutoOffset::UpdateReference() {

	// Verifica apenas se a referência existe
	// m_Offset pode ser 0 (offset válido) ou diferente de 0
	if (!m_Reference)
		return false;

	// Se chegou aqui, encontrou o padrão e tem um offset
	// (mesmo que seja 0x00000000)
	*m_Reference = m_Offset;

	return true;
}

void AutoOffset::SetReference(INT64* Reference) {
	m_Reference = Reference;
}

void AutoOffset::SetPattern(PBYTE Pattern) {
	m_Pattern = Pattern;
}

void AutoOffset::SetMask(const char* Mask) {
	m_Mask = Mask;
}

void AutoOffset::SetSection(const char* Section) {
	m_Section = Section;
}

void AutoOffset::SetType(ScanType Type) {
	m_Type = Type;
}

void AutoOffset::SetOffset(UINT32 Offset) {
	m_InstructionOffset = Offset;
}

void AutoOffset::SetLastOffset(INT32 Offset) {
	m_LastOffset = Offset;
}

INT64 AutoOffset::GetOffset() const {
	return m_Offset;
}

bool AutoOffset::Scan(UINT64 Module, PBYTE Allocated) {

	auto Instruction = Utils::PatternScan(
		Module,
		Allocated,
		m_Section,
		m_Pattern,
		m_Mask
	);

	if (!Instruction) /* oh no ! */ {
		printf("[UPDATER] ❌ Padrão não encontrado na seção %s\n", m_Section);
		return false;
	}

	return ResolveOffset(Module, Instruction);
}

bool Updater::AllocateModule() {

	DWORD Pid = 0;

	printf("[UPDATER] Procurando DayZ_x64.exe...\n");
	if (!Utils::GetProcessId("DayZ_x64.exe", &Pid)) {
		printf("[UPDATER] ❌ ERRO: DayZ_x64.exe não encontrado! Verifique se o jogo está aberto.\n");
		return false;
	}

	printf("[UPDATER] ✅ Processo encontrado! PID: %lu\n", Pid);

	printf("[UPDATER] Carregando módulo...\n");
	if (!Utils::GetProcessBase(Pid, &m_Module, &m_Allocated)) {
		printf("[UPDATER] ❌ ERRO: Falha ao carregar módulo do processo.\n");
		return false;
	}

	printf("[UPDATER] ✅ Módulo carregado com sucesso!\n");
	printf("[UPDATER]    - Endereço base: 0x%llx\n", m_Module);
	printf("[UPDATER]    - Buffer alocado: 0x%p\n", m_Allocated);

	return true;
}

bool Updater::DeallocateModule() {

	FreeLibrary((HMODULE)(m_Module));

	m_Module = NULL;
	m_Allocated = NULL;

	return true;
}

void Updater::SetupModbasePatterns() {

	AUTO_OFFSET(Modbase, World, "\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x54\x24\x00\x48\x8B\x48\x30", "xxx????xxxx?xxxx", ".text", ScanType::MovCs, 0);
	AUTO_OFFSET(Modbase, Network, "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x1D\x00\x00\x00\x00\x84\xC0", "xxx????x????xxx????xx", ".text", ScanType::MovCs, 0);
	AUTO_OFFSET(Modbase, Tick, "\x48\x8B\x05\x00\x00\x00\x00\x0F\x57\xC9\x66\x0F\x6E\x03", "xxx????xxxxxxx", ".text", ScanType::MovCs, 0);

}


void Updater::SetupNetworkPatterns() {

	// TODO: Padrões para Network::Scoreboard necessitam de análise em Ghidra

}

void Updater::SetupPlayerIdentityPatterns() {

	// TODO: Padrões para PlayerIdentity necessitam de análise em Ghidra

}

void Updater::SetupWorldPatterns() {

	// BulletList - expandido com contexto adicional para evitar false positives
	AUTO_OFFSET(World, BulletList, "\x48\x8B\x83\x00\x00\x00\x00\x49\x8B\xCF\x48\x03\x0C\xF8\x48\x8B\x48", "xxx????xxxxxxxxxx", ".text", ScanType::MovReg, 0);

	// NearEntList - expandido para evitar matches genéricos
	AUTO_OFFSET(World, NearEntList, "\x48\x8B\x83\x00\x00\x00\x00\x49\x8B\x14\x06\x48\x3B\xD5\x75\x0D", "xxx????xxxxxxxxxx", ".text", ScanType::MovReg, 0);

	// FarEntList - padrão longo mantido para evitar false positives
	AUTO_OFFSET(World, FarEntList, "\x48\x8B\x83\x00\x00\x00\x00\x49\x8B\x0C\x06\x48\x3B\xCD\x74\x17\x80\xB9\x00\x00\x00\x00\x00\x75\x0E\x41\xB8\x00\x00\x00\x00\x0F\x28\xCE\xE8\x00\x00\x00\x00\xFF\xC6\x49\x83\xC6\x08\x3B\xB3\x00\x00\x00\x00\x7C\xCB", "xxx????xxxxxxxxxxx?????xxxx????xxxx????xxxxxxxx????xx", ".text", ScanType::MovReg, 0);

	// Camera - expandido com mais contexto
	AUTO_OFFSET(World, Camera, "\x4C\x8B\x83\x00\x00\x00\x00\x4C\x8B\x11\x48\x89\x70\x08\x48\x8D", "xxx????xxxxxxxxxx", ".text", ScanType::MovReg, 0);

	// LocalPlayer - padrão com trace - mantido como estava
	AUTO_OFFSET(World, LocalPlayer, "\xE8\x00\x00\x00\x00\x48\x8B\xC8\xC7\x44\x24\x00\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00", "x????xxxxxx?????xxx????", ".text", ScanType::TraceMovReg, 0);

	// LocalOffset - padrão com trace - mantido como estava
	AUTO_OFFSET(World, LocalOffset, "\xE8\x00\x00\x00\x00\x48\x8B\xC8\xC7\x44\x24\x00\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00", "x????xxxxxx?????xxx????", ".text", ScanType::TraceMovReg, 16);

}

void Updater::SetupHumanPatterns() {

	// HumanType - expandido com contexto para evitar false positives
	AUTO_OFFSET(Human, HumanType, "\x4C\x8B\xB1\x00\x00\x00\x00\x32\xDB\x0F\x29\x74\x24\x00\x48\x85", "xxx????xxxxxxxxxx", ".text", ScanType::MovReg, 0);

	// VisualState - expandido com más contexto
	AUTO_OFFSET(Human, VisualState, "\x48\x8B\xB7\x00\x00\x00\x00\x45\x33\xFF\xF7\xE9\x85\xC0", "xxx????xxxxxxxx", ".text", ScanType::MovReg, 0);

	// LodShape - expandido com contexto adicional
	AUTO_OFFSET(Human, LodShape, "\x4C\x8B\x91\x00\x00\x00\x00\x49\x8B\xF9\x48\x63\xC2\x48\x63", "xxx????xxxxxxxx", ".text", ScanType::MovReg, 0);

}

void Updater::SetupDayZInfectedPatterns() {



}

void Updater::SetupHumanTypePatterns() {

	AUTO_OFFSET(HumanType, ObjectName, "\x48\x8B\x58\x70\x48\x85\xDB\x74\x03\xF0\xFF\x03\x48\x8B\x44\x24\x00\xBE\x00\x00\x00\x00\x48\x89\x5C\x24\x00\xEB\x21\x48\x8D\x0D\x00\x00\x00\x00", "xxxxxxxxxxxxxxxx?x????xxxx?xxxxx????", ".text", ScanType::MovRegByte, 0);
	AUTO_OFFSET(HumanType, CategoryName, "\x48\x8B\x81\x00\x00\x00\x00\x48\x8B\xF9\x0F\xB6\xF2\x48\x8D\x48\x10\x48\x85\xC0\x75\x07\x48\x8D\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x89\x6C\x24\x00\xE8\x00\x00\x00\x00", "xxx????xxxxxxxxxxxxxxxxxx????xxx????xxxx?x????", ".text", ScanType::MovReg, 0);

}

void Updater::SetupDayZLocalPatterns() {

	AUTO_OFFSET(DayZInfected, Skeleton, "\x48\x8B\x89\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x84\xC0\x74\x31", "xxx????x????xxxx", ".text", ScanType::MovReg, 0);

}

void Updater::SetupDayZPlayerPatterns() {

	AUTO_OFFSET(DayZPlayer, Skeleton, "\x49\x8B\x97\x00\x00\x00\x00\x48\x8D\x4D\xD0", "xxx????xxxx", ".text", ScanType::MovReg, 0);
	AUTO_OFFSET(DayZPlayer, NetworkID, "\x41\x8B\x9E\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xC8", "xxx????x????xxx", ".text", ScanType::MovReg, 0);
	AUTO_OFFSET(DayZPlayer, Inventory, "\x48\x8B\x8B\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00\xEB\x02", "xxx????xxxxx????xx", ".text", ScanType::MovReg, 0);

}

void Updater::SetupDayZPlayerInventoryPatterns() {

	AUTO_OFFSET(DayZPlayerInventory, Hands, "\x48\x8B\x8B\x00\x00\x00\x00\x48\x8B\xF8\x48\x85\xC9", "xxx????xxxxxx", ".text", ScanType::MovReg, 0);

}

void Updater::SetupInventoryItemPatterns() {

	AUTO_OFFSET(InventoryItem, ItemInventory, "\x48\x8B\x8B\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00\xEB\x02", "xxx????xxxxx????xx", ".text", ScanType::MovReg, 0);


}

void Updater::SetupWeaponPatterns() {

	AUTO_OFFSET(Weapon, WeaponIndex, "\x48\x8B\xCF\x48\x63\x9F\x00\x00\x00\x00\xFF\x90\x00\x00\x00\x00", "xxxxxx????xx????", ".text", ScanType::MovReg, 3);
	AUTO_OFFSET(Weapon, WeaponInfoTable, "\x48\x03\x91\x00\x00\x00\x00\x48\x83\x7A\x00\x00\x74\x19", "xxx????xxx??xx", ".text", ScanType::MovReg, 0);
	AUTO_OFFSET(Weapon, MuzzleCount, "\x3B\x98\x00\x00\x00\x00\x73\x00\x8B\xCB", "xx????x?xx", ".text", ScanType::MovRegSml, 0);
	AUTO_OFFSET(Weapon, WeaponInfoSize, "\x48\x69\xD0\x00\x00\x00\x00\x48\x03\x91\x00\x00\x00\x00\x48\x83\x7A\x00\x00", "xxx????xxx????xxx??", ".text", ScanType::MovReg, 0);

}

void Updater::SetupWeaponInventoryPatterns() {

	AUTO_OFFSET(WeaponInventory, MagazineRef, "\x48\x8B\xB9\x00\x00\x00\x00\x48\x8B\xE9\x8B\xB1\x00\x00\x00\x00", "xxx????xxxxx????", ".text", ScanType::MovReg, 0);

}

void Updater::SetupMagazinePatterns() {

	AUTO_OFFSET(Magazine, MagazineType, "\x4C\x8B\xB1\x00\x00\x00\x00\x32\xDB\x0F\x29\x74\x24\x00", "xxx????xxxxxx?", ".text", ScanType::MovReg, 0);
	AUTO_OFFSET(Magazine, AmmoCount, "\x8B\xA9\x00\x00\x00\x00\x4C\x89\x74\x24\x00", "xx????xxxx?", ".text", ScanType::MovRegSml, 0);

}

void Updater::SetupAmmoTypePatterns() {

	AUTO_OFFSET(AmmoType, InitSpeed, "\x45\x0F\x2F\x8F\x00\x00\x00\x00\x0F\x83\x00\x00\x00\x00", "xxxx????xx????", ".text", ScanType::MovRegXmm, 0);
	AUTO_OFFSET(AmmoType, AirFriction, "\xF3\x45\x0F\x10\x87\x00\x00\x00\x00\x48\x8D\x55\xC7", "xxxxx????xxxx", ".text", ScanType::MovRegXmmLrg, 0);

}

void Updater::SetupSkeletonPatterns() {

	// AnimClass1 - expandido para evitar matches incorretos
	AUTO_OFFSET(Skeleton, AnimClass1, "\x48\x83\xC1\x70\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8B\x49\x28\x48\x85\xC9\x75\x03\x32\xC0\x90", "xxxxx????xxxxxxxxxxxxxxxxxx", ".text", ScanType::MovRegByte, 0);

	// AnimClass2 - expandido com mais contexto
	AUTO_OFFSET(Skeleton, AnimClass2, "\xE8\x00\x00\x00\x00\xEB\x12\x4C\x8B\xCB\x89\x7C\x24\x20\x4D\x8B\xC4\x49\x8B\xCE\x45", "x????xxxxxxxxxxxxxxxxxxxxx", ".text", ScanType::TraceMovRegByte, 16);

}

void Updater::SetupAnimClassPatterns() {

	AUTO_OFFSET(AnimClass, MatrixArray, "\x49\x8B\xBE\x00\x00\x00\x00\x83\xFA\xFF\x74\x00", "xxx????xxxx?", ".text", ScanType::MovReg, 0);

}

void Updater::SetupCameraPatterns() {

	// ViewMatrix - expandido para evitar false positives
	AUTO_OFFSET(Camera, ViewMatrix, "\xF3\x0F\x10\x40\x00\xF3\x0F\x10\x50\x00\xF3\x0F\x10\x58\x00\x0F", "xxxx?xxxx?xxxx?x", ".text", ScanType::MovRegXmmByte, 0);

	// ViewPortMatrix - expandido com contexto
	AUTO_OFFSET(Camera, ViewPortMatrix, "\xF3\x0F\x11\x4E\x00\x66\x0F\x6E\xC1\x0F", "xxxx?xxxxx", ".text", ScanType::MovRegXmmByte, 0);

	// ViewProjection - padrão longo mantido (já está bom)
	AUTO_OFFSET(Camera, ViewProjection, "\x0F\x11\x86\x00\x00\x00\x00\x0F\x10\x44\x24\x00\x0F\x11\x86\x00\x00\x00\x00\x0F\x10\x44\x24\x00\x0F\x11\x86\x00\x00\x00\x00\x48\x8B\x06", "xxx????xxxx?xxx????xxxx?xxx????xxx", ".text", ScanType::MovRegByte, 0);

}

void Updater::SetupVisualStatePatterns() {

	// Transform - expandido com contexto para evitar duplicatas
	AUTO_OFFSET(VisualState, Transform, "\xF3\x0F\x10\x40\x00\xF3\x0F\x10\x50\x00\xF3\x0F\x10\x58\x00\x0F", "xxxx?xxxx?xxxx?x", ".text", ScanType::MovRegXmmByte, 0);

	// InverseTransform - expandido com contexto
	AUTO_OFFSET(VisualState, InverseTransform, "\x89\x8B\x00\x00\x00\x00\x8B\x4A\x04\x48", "xx????xxxx", ".text", ScanType::MovRegSml, 0);

}


bool Updater::SetupPatterns() {

	printf("[UPDATER] Registrando padrões de busca...\n");

	int initialCount = m_Scans.size();

	SetupModbasePatterns();
	SetupNetworkPatterns();
	SetupPlayerIdentityPatterns();
	SetupWorldPatterns();
	SetupHumanPatterns();
	SetupDayZInfectedPatterns();
	SetupHumanTypePatterns();
	SetupDayZLocalPatterns();
	SetupDayZPlayerPatterns();
	SetupDayZPlayerInventoryPatterns();
	SetupInventoryItemPatterns();
	SetupWeaponPatterns();
	SetupWeaponInventoryPatterns();
	SetupMagazinePatterns();
	SetupAmmoTypePatterns();
	SetupSkeletonPatterns();
	SetupAnimClassPatterns();
	SetupCameraPatterns();
	SetupVisualStatePatterns();

	int totalCount = m_Scans.size();
	printf("[UPDATER] ✅ %d padrões registrados com sucesso!\n\n", totalCount);

	return true;
}

void Updater::SaveOffsetsToFile() {

	std::ofstream file("Offsets_Result.txt");

	if (!file.is_open()) {
		printf("[UPDATER] ❌ ERRO: Falha ao criar arquivo Offsets_Result.txt\n");
		return;
	}

	printf("[UPDATER] 📝 Salvando offsets em arquivo...\n\n");

	file << "================================\n";
	file << "       DAYZ OFFSETS - RESULT      \n";
	file << "================================\n\n";

#if _DEBUG
	for (const auto& Data : m_Scans) {
		INT64 offset = Data.second.GetOffset();

		// Salvar todos os offsets (inclusive zero)
		if (offset != 0) {
			file << Data.first << " = 0x" << std::hex << offset << std::dec << "\n";
			printf("[UPDATER] 📄 Salvo: %s = 0x%llx\n", Data.first.c_str(), offset);
		} else {
			file << Data.first << " = 0x0 (ZERO - INVÁLIDO)\n";
			printf("[UPDATER] 📄 Salvo (zero): %s\n", Data.first.c_str());
		}
	}
#else
	// Para release, salvamos somente os offsets diretamente
	for (const auto& Data : m_Scans) {
		INT64 offset = Data.GetOffset();

		if (offset != 0) {
			file << "Offset = 0x" << std::hex << offset << std::dec << "\n";
			printf("[UPDATER] 📄 Salvo = 0x%llx\n", offset);
		}
	}
#endif

	file << "\n================================\n";
	file << "          FIM DO ARQUIVO         \n";
	file << "================================\n";

	file.close();

	printf("[UPDATER] ✅ Arquivo Offsets_Result.txt criado com sucesso!\n\n");
}

void Updater::SaveDebugReport() {

	std::ofstream file("Offsets_Debug_Report.txt");

	if (!file.is_open()) {
		printf("[UPDATER] ❌ ERRO: Falha ao criar arquivo de debug\n");
		return;
	}

	printf("[UPDATER] 📋 Gerando relatório detalhado de debug...\n\n");

	file << "╔════════════════════════════════════════════════════════════╗\n";
	file << "║          DAYZ OFFSETS - DEBUG REPORT                      ║\n";
	file << "║     COM SUPORTE A PONTEIROS DINÂMICOS MULTINÍVEL          ║\n";
	file << "╠════════════════════════════════════════════════════════════╣\n";
	file << "║ INFORMAÇÕES DO MÓDULO                                      ║\n";
	file << "║ ────────────────────────────────────────────────────────── ║\n";
	file << "║ Endereço Base: 0x" << std::hex << m_Module << std::dec << "                          ║\n";
	file << "║ Buffer Alocado: 0x" << std::hex << (UINT64)m_Allocated << std::dec << "                          ║\n";
	file << "╠════════════════════════════════════════════════════════════╣\n";
	file << "║ STATUS DOS OFFSETS (COM ANÁLISE DE DINÂMICOS)             ║\n";
	file << "║ ────────────────────────────────────────────────────────── ║\n\n";

#if _DEBUG
	int found = 0, notFound = 0, zero = 0, dynamic = 0;

	for (const auto& Data : m_Scans) {
		INT64 offset = Data.second.GetOffset();

		file << "┌─ " << Data.first << "\n";

		if (offset == -1 || offset == 0xFFFFFFFFFFFFFFFF) {
			file << "│  Status: ❌ NÃO ENCONTRADO\n";
			file << "│  Offset: INVÁLIDO\n";
			file << "│  Razão: Padrão não localizado na seção de código\n";
			notFound++;
		} else if (offset == 0) {
			file << "│  Status: ⚠️ ZERO - ANALISANDO COMO DINÂMICO...\n";
			file << "│  Offset: 0x0\n";
			file << "│  Tentando: Dereferência como ponteiro dinâmico\n";

			// Tenta resolver como ponteiro
			INT64 DynamicPtr = 0;
			if (TryResolvePointerChain(m_Allocated, 0, DynamicPtr)) {
				file << "│  ✅ ENCONTRADO COMO DINÂMICO:\n";
				file << "│     Ponteiro em: 0x0\n";
				file << "│     Aponta para: 0x" << std::hex << DynamicPtr << std::dec << "\n";
				file << "│     Tipo: MULTINÍVEL - Dereference em tempo de execução\n";
				dynamic++;
			} else {
				file << "│  ❌ Não é um ponteiro válido\n";
				zero++;
			}
		} else {
			file << "│  Status: ✅ VÁLIDO (ESTÁTICO)\n";
			file << "│  Offset: 0x" << std::hex << offset << std::dec << "\n";
			file << "│  Tipo: DIRETO - Endereço fixo\n";
			found++;
		}
		file << "└─\n\n";
	}

	file << "╠════════════════════════════════════════════════════════════╣\n";
	file << "║ RESUMO ESTATÍSTICO                                         ║\n";
	file << "║ ────────────────────────────────────────────────────────── ║\n";
	file << "║ Total de Offsets: " << found + notFound + zero + dynamic << "                             ║\n";
	file << "║ Estáticos Diretos (✅): " << found << "                             ║\n";
	file << "║ Dinâmicos Multinível (🔗): " << dynamic << "                             ║\n";
	file << "║ Não Encontrados (❌): " << notFound << "                             ║\n";
	file << "║ Problemas (⚠️): " << zero << "                             ║\n";
	file << "║ Taxa de Sucesso: " << ((found + dynamic) * 100 / (found + notFound + zero + dynamic)) << "%                          ║\n";

#else
	int found = 0, notFound = 0, zero = 0, dynamic = 0;

	for (const auto& Data : m_Scans) {
		INT64 offset = Data.GetOffset();

		if (offset == 0) {
			zero++;
		} else if (offset < 0) {
			notFound++;
		} else {
			found++;
		}
	}

	file << "Offsets Válidos: " << found << "\n";
	file << "Offsets Inválidos: " << notFound << "\n";
	file << "Offsets Zero: " << zero << "\n";
#endif

	file << "╠════════════════════════════════════════════════════════════╣\n";
	file << "║ GUIA DE USO                                                ║\n";
	file << "║ ────────────────────────────────────────────────────────── ║\n";
	file << "║ ✅ ESTÁTICOS DIRETOS:                                      ║\n";
	file << "║    Use direto: (UINT64)pModule + Offset                   ║\n";
	file << "║    Exemplo: World = *(INT64*)((UINT64)pModule + 0x12AB43) ║\n";
	file << "║                                                            ║\n";
	file << "║ 🔗 DINÂMICOS MULTINÍVEL:                                   ║\n";
	file << "║    1. Carregue o ponteiro: ptrPtr = *(INT64*)offset       ║\n";
	file << "║    2. Dereference: Ptr = *(INT64*)ptrPtr                  ║\n";
	file << "║    3. Acesse membro: Membro = *(INT64*)(Ptr + 0x38)       ║\n";
	file << "║    Exemplo para Network::Scoreboard:                      ║\n";
	file << "║    pNetwork = *(INT64*)((INT64*)pModule + offset)         ║\n";
	file << "║    Scoreboard = *(INT64*)(pNetwork + 0x50)                ║\n";
	file << "║                                                            ║\n";
	file << "║ ⚠️ OFFSETS ZERO:                                           ║\n";
	file << "║    - Padrão encontrado mas cálculo falhou                 ║\n";
	file << "║    - Tente atualizar a máscara (mask)                     ║\n";
	file << "║    - Use Ghidra para verificar a instrução                ║\n";
	file << "╠════════════════════════════════════════════════════════════╣\n";
	file << "║ RECOMENDAÇÕES                                              ║\n";
	file << "║ ────────────────────────────────────────────────────────── ║\n";
	file << "║ 1. Offsets ✅ Estáticos: Use direto no seu código         ║\n";
	file << "║                                                            ║\n";
	file << "║ 2. Offsets 🔗 Dinâmicos: Implementar cadeia de ponteiros  ║\n";
	file << "║    Será necessário desreferenciar múltiplas vezes         ║\n";
	file << "║                                                            ║\n";
	file << "║ 3. Offsets ❌ Não Encontrados ou ⚠️ Zero:                  ║\n";
	file << "║    - Jogo pode ter sido atualizado                        ║\n";
	file << "║    - Use Ghidra para localizar novo padrão                ║\n";
	file << "║    - Atualize em SetupPatterns()                          ║\n";
	file << "╚════════════════════════════════════════════════════════════╝\n";

	file.close();

	printf("[UPDATER] ✅ Arquivo Offsets_Debug_Report.txt criado!\n\n");
}

bool Updater::TryResolvePointerChain(PBYTE Module, INT64 PointerOffset, INT64& OutPointerValue) {

	if (PointerOffset <= 0 || PointerOffset > 0x1000000) {
		return false; // Offset inválido
	}

	try {
		// Lê o valor armazenado no offset do ponteiro (deve ser um endereço de memória)
		PBYTE PointerAddr = Module + PointerOffset;
		INT64 DynamicPtr = *(INT64*)(PointerAddr);

		// Valida se parece um endereço válido
		// Endereços válidos tipicamente > 0x10000 e < 0x00007FFFFFFFFFFF
		if (DynamicPtr > 0x10000 && DynamicPtr < 0x00007FFFFFFFFFFF) {
			OutPointerValue = DynamicPtr;
			return true;
		}
	}
	catch (...) {
		return false;
	}

	return false;
}

bool Updater::Init() {

	if (!AllocateModule())
		return false;

	if (!SetupPatterns())
		return false;

	return true;
}

bool Updater::Scan() {

	int scanned = 0;
	int found = 0;
	int notFound = 0;
	int zeroOffsets = 0;

	for (auto& Data : m_Scans) {

#if _DEBUG
		if (!Data.second.Scan(m_Module, m_Allocated)) {
			notFound++;
			scanned++;
		} else {
			scanned++;
			if (Data.second.GetOffset() == 0) {
				printf("[UPDATER] ⚠️  OFFSET ZERO: %s\n", Data.first.c_str());
				zeroOffsets++;
			} else {
				printf("[UPDATER] ✅ VÁLIDO: %s = 0x%llx\n", Data.first.c_str(), Data.second.GetOffset());
				found++;
			}
		}
#else 
		if (!Data.Scan(m_Module, m_Allocated)) {
			notFound++;
			scanned++;
		} else {
			scanned++;
			if (Data.GetOffset() == 0) {
				printf("[UPDATER] ⚠️  OFFSET ZERO\n");
				zeroOffsets++;
			} else {
				printf("[UPDATER] ✅ VÁLIDO = 0x%llx\n", Data.GetOffset());
				found++;
			}
		}
#endif

	}

	printf("\n[UPDATER] ════════════════════════════════\n");
	printf("[UPDATER] Scan concluído:\n");
	printf("[UPDATER]   - Total processado: %d\n", scanned);
	printf("[UPDATER]   - Encontrados (não-zero): %d ✅\n", found);
	printf("[UPDATER]   - Offsets zero (inválidos): %d ⚠️\n", zeroOffsets);
	printf("[UPDATER]   - Não encontrados: %d ❌\n", notFound);
	printf("[UPDATER] ════════════════════════════════\n\n");

	return true;
}

/* cluster fuck #if */
bool Updater::Release() {

	bool Result = true;
	int successCount = 0;  // Apenas offsets não-zero
	int failureCount = 0;  // Offsets zero e não-encontrados

	printf("[UPDATER] Salvando offsets válidos...\n\n");

	for (auto& Data : m_Scans) {
#if _DEBUG
		if (!Data.second.UpdateReference()) {
			printf("[UPDATER] ❌ FALHOU: %s\n", Data.first.c_str());
			failureCount++;
		} else {
			INT64 offset = Data.second.GetOffset();

			// Filtrar: só mostrar offsets não-zero
			if (offset != 0) {
				printf("[UPDATER] ✅ SALVO: %s = 0x%llx\n", Data.first.c_str(), offset);
				successCount++;
			} else {
				printf("[UPDATER] ⚠️  ZERO (ignorado): %s\n", Data.first.c_str());
				failureCount++;
			}
		}
#else
		if (!Data.UpdateReference()) {
			printf("[UPDATER] ❌ FALHOU\n");
			failureCount++;
		} else {
			INT64 offset = Data.GetOffset();

			// Filtrar: só mostrar offsets não-zero
			if (offset != 0) {
				printf("[UPDATER] ✅ SALVO = 0x%llx\n", offset);
				successCount++;
			} else {
				printf("[UPDATER] ⚠️  ZERO (ignorado)\n");
				failureCount++;
			}
		}
#endif

	}

	printf("\n");
	printf("╔═══════════════════════════════════╗\n");
	printf("║      RESUMO FINAL                 ║\n");
	printf("║ Total processado:      %2d         ║\n", successCount + failureCount);
	printf("║ Salvos (válidos):      %2d ✅      ║\n", successCount);
	printf("║ Ignorados/Falhados:    %2d ⚠️      ║\n", failureCount);
	printf("╚═══════════════════════════════════╝\n\n");

	// Salvar offsets em arquivo TXT
	SaveOffsetsToFile();

	// Salvar relatório de debug
	SaveDebugReport();

	m_Scans.clear();

	(void)DeallocateModule();

	return Result;
}
