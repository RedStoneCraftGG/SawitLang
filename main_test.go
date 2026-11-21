package main

import (
	"strings"
	"testing"
)

func mustContain(t *testing.T, out string, wants ...string) {
	t.Helper()
	for _, want := range wants {
		if !strings.Contains(out, want) {
			t.Errorf("output missing %q\ngot:\n%s", want, out)
		}
	}
}

func mustNotContain(t *testing.T, out string, bads ...string) {
	t.Helper()
	for _, bad := range bads {
		if strings.Contains(out, bad) {
			t.Errorf("output should not contain %q\ngot:\n%s", bad, out)
		}
	}
}

func TestKeywordAndFunctionMapping(t *testing.T) {
	out := transpile("tandan panenUtama() { hutan x; buzzer(\"hi\"); setor 0; }")
	mustContain(t, out, "int main()", "void x;", "sawit_buzzer(", "return 0;")
}

func TestDoWhileMapping(t *testing.T) {
	out := transpile("pidato { setor 0; } janji (sesuai);")
	mustContain(t, out, "do {", "} while (true);")
}

func TestCommentsAndStringsPreserved(t *testing.T) {
	src := "// tandan panenUtama buzzer\n" +
		"tandan panenUtama() {\n" +
		"    buzzer(\"tandan panenUtama buzzer\\n\");\n" +
		"    setor 0;\n}\n"
	out := transpile(src)
	mustContain(t, out,
		"// tandan panenUtama buzzer",
		"\"tandan panenUtama buzzer\\n\"",
		"int main()",
	)
}

func TestIdentifiersNotSplit(t *testing.T) {
	out := transpile("tandan tandananku = 1;")
	mustContain(t, out, "int tandananku")
}

func TestConstructorKeywordsRemoved(t *testing.T) {
	out := transpile("tanam() { tebang(); }")
	mustNotContain(t, out, "constructor", "destructor")
}

func TestAllocatorMappedToRuntime(t *testing.T) {
	out := transpile("hutan *p = beli(8); jual(p);")
	mustContain(t, out, "sawit_beli(8)", "sawit_jual(p)")
	mustNotContain(t, out, "malloc", "free(")
}

func TestUnterminatedStringDoesNotPanic(t *testing.T) {
	defer func() {
		if r := recover(); r != nil {
			t.Fatalf("transpile panicked on unterminated string: %v", r)
		}
	}()
	_ = transpile("buzzer(\"abc\\")
}

func TestUnterminatedCommentDoesNotPanic(t *testing.T) {
	defer func() {
		if r := recover(); r != nil {
			t.Fatalf("transpile panicked on unterminated comment: %v", r)
		}
	}()
	_ = transpile("/* unclosed")
}

func TestTranspileAlwaysIncludesRuntime(t *testing.T) {
	out := transpile("")
	mustContain(t, out, `#include "sawit_runtime.h"`)
}

func TestSystemVocabularyMapping(t *testing.T) {
	out := transpile(`
    keping8 a; keping16 b; keping32 c; keping64 d; alamat p;
    menguap keping16 *reg; tetap int k; statis int s; luar int e;
    segera int f(); takTanda n; kosong; seberapa(n); rumpun R { A };
    serikat U { int x; }; sebutan int Angka;
    sadap("%d", &n); sadapBaris(buf, 8); sadapKarakter(); sadapSisa();
    tutupTelinga(); bukaTelinga(); samaKata(a, b); panjangKata(a);
    terjemahAngka(a); salinKata(a, b);
    semprot(p, 1); hisap(p); kunciInterupsi(); bukaInterupsi();
    hentiMesin(); tulisLayar(a);
    pasangMesin(100); tulisSerial(a); tunda(1); bacaTick(); gentar(a);
    infoMultiboot(); infoRAM(); infoPetaMemori(&p, &n);
    nyalakanUlang(); sadapScancode(); ukuranBlok(p); tulisWarna(7, a);
    pasangHalaman(); halamanAktif(); frameTotal(); frameBebas();
    mintaFrame(); kembalikanFrame(p); prosesBaru(f, p); prosesYield();
    prosesKeluar(); prosesId(); prosesJumlah(); panggilSistem(0, 1, 2, 3);
    pasangCakram(); adaCakram(cakramRAM); sektorCakram(cakramRAM);
    bacaSektor(cakramRAM, 0, p); tulisSektor(cakramRAM, 0, p);
    formatCakram(cakramRAM); pasangBerkas(cakramRAM); terpasangBerkas();
    blokTotal(); blokBebas(); buatBerkas(a); bukaBerkas(a);
    bacaBerkas(0, p, 16); tulisBerkas(0, p, 16); hapusBerkas(a);
    ukuranBerkas(a); daftarBerkas(); layananBaca(0, p, 8);
    layananTulis(0, p, 8); layananBuka(a); layananDaftar();
    prosesUserBaru(); prosesUserJumlah(); prosesRing();
`)
	mustContain(t, out,
		"uint8_t a;", "uint16_t b;", "uint32_t c;", "uint64_t d;",
		"uintptr_t p;", "volatile uint16_t *reg;", "const int k;",
		"static int s;", "extern int e;", "inline int f();",
		"unsigned n;", "NULL;", "sizeof(n);", "enum R { A };",
		"union U { int x; };", "typedef int Angka;",
		"sawit_sadap(", "sawit_sadap_baris(", "sawit_sadap_karakter()",
		"sawit_sadap_sisa()", "sawit_tutup_telinga()", "sawit_buka_telinga()",
		"sawit_sama_kata(", "sawit_panjang_kata(", "sawit_terjemah_angka(",
		"sawit_salin_kata(", "sawit_outb(p, 1)", "sawit_inb(p)",
		"sawit_cli()", "sawit_sti()", "sawit_hlt()", "sawit_tulis_layar(a)",
		"sawit_pasang_mesin(100)", "sawit_serial_write(a)", "sawit_tunda(1)",
		"sawit_tick()", "sawit_panic(a)", "sawit_mb_ok()", "sawit_mb_ram_kb()",
		"sawit_mb_largest_usable(&p, &n)", "sawit_nyalakan_ulang()",
		"sawit_sadap_scancode()", "sawit_blok_ukuran(p)", "sawit_tulis_warna(7, a)",
		"sawit_paging_init()", "sawit_paging_ok()", "sawit_pmm_total()",
		"sawit_pmm_free()", "sawit_pmm_alloc_frame()", "sawit_pmm_free_frame(p)",
		"sawit_process_create(f, p)", "sawit_yield()", "sawit_process_exit()",
		"sawit_process_current_id()", "sawit_process_count()", "sawit_syscall(0, 1, 2, 3)",
		"sawit_ata_init()", "sawit_blk_ok(SAWIT_DEV_RAM)",
		"sawit_blk_sectors(SAWIT_DEV_RAM)",
		"sawit_blk_read(SAWIT_DEV_RAM, 0, p)", "sawit_blk_write(SAWIT_DEV_RAM, 0, p)",
		"sawit_fs_format(SAWIT_DEV_RAM)", "sawit_fs_mount(SAWIT_DEV_RAM)",
		"sawit_fs_mounted()", "sawit_fs_total_blocks()", "sawit_fs_free_blocks()",
		"sawit_fs_create(a)", "sawit_fs_open(a)", "sawit_fs_read(0, p, 16)",
		"sawit_fs_write(0, p, 16)", "sawit_fs_remove(a)", "sawit_fs_size(a)",
		"sawit_fs_list()", "sawit_sys_read(0, p, 8)", "sawit_sys_write(0, p, 8)",
		"sawit_sys_open(a)", "sawit_sys_list()",
		"sawit_user_spawn()", "sawit_user_count()", "sawit_current_ring()",
	)
}

func TestRampasMappingAndAlias(t *testing.T) {
	out := transpile("rampas(&q, &p); sita(&q, &p);")
	mustContain(t, out, "sawit_rampas(&q, &p);", "sawit_rampas(&q, &p);")
	if strings.Count(out, "sawit_rampas(") != 2 {
		t.Errorf("rampas dan sita harus sama-sama masuk sawit_rampas, got:\n%s", out)
	}
}

func TestBootAssetsEmbedded(t *testing.T) {
	if !strings.Contains(string(bootStart), "0xE85250D6") || !strings.Contains(string(bootStart), "_start:") {
		t.Errorf("boot/start.S tidak ter-embed dengan benar")
	}
	if !strings.Contains(string(bootLinker), "ENTRY(_start)") || !strings.Contains(string(bootLinker), "1M") {
		t.Errorf("boot/linker.ld tidak ter-embed dengan benar")
	}
	if !strings.Contains(string(bootStart), "sawit_mb_magic") || !strings.Contains(string(bootStart), "sawit_mb_info") {
		t.Errorf("start.S belum menyimpan informasi Multiboot")
	}
	for _, symbol := range []string{"sawit_pasang_mesin", "sawit_mb_largest_usable", "sawit_sadap_baris", "sawit_blok_ukuran", "sawit_paging_init", "sawit_process_create", "sawit_context_switch", "sawit_syscall", "sawit_ata_init", "sawit_fs_format", "sawit_fs_mount", "sawit_fs_read", "sawit_fs_write", "sawit_ramdisk"} {
		if !strings.Contains(string(kernelHeader), symbol) {
			t.Errorf("runtime kernel kehilangan %s", symbol)
		}
	}
}

func TestSyscallStubFrame(t *testing.T) {
	h := string(kernelHeader)
	for _, pattern := range []string{
		"sawit_syscall_dispatch",
		"__attribute__((naked)) static void sawit_isr_syscall(void)",
		"pushl $128",
		"pusha",
		"call sawit_syscall_dispatch",
		"iret",
	} {
		if !strings.Contains(h, pattern) {
			t.Errorf("stub syscall kehilangan %q", pattern)
		}
	}
	// Regresi: atribut interrupt GCC hanya menaruh EIP, bukan frame lengkap.
	// Fallback COFF (compile-check Windows) boleh memakainya, tapi hanya jika
	// berada di balik penanda #else yang jelas.
	buggy := "__attribute__((interrupt)) sawit_isr_syscall(void *raw)"
	if idx := strings.Index(h, buggy); idx >= 0 && !strings.Contains(h[:idx], "Compile-check Windows (COFF)") {
		t.Errorf("handler syscall interrupt tidak berada di fallback COFF")
	}
}

func TestUserlandPrimitives(t *testing.T) {
	h := string(kernelHeader)
	for _, pattern := range []string{
		"sawit_idt_set_user", "0xEE",
		"__attribute__((naked)) static void sawit_context_switch_user",
		"0x23u", "0x1Bu", "0x202u",
		"sawit_page_allow_user", "|= 4u", "invlpg",
		"sawit_page_dir[0] = (uint32_t)(uintptr_t)&sawit_page_table_low | 7u",
		"movl $0x23, %%eax", "movl $0x10, %%eax",
		"sawit_user_ptr_ok", "sawit_user_spawn",
		"sawit_user_payload", "int $0x80",
		"sawit_user_count", "sawit_current_ring",
	} {
		if !strings.Contains(h, pattern) {
			t.Errorf("runtime userland kehilangan %q", pattern)
		}
	}
}

func TestFreestandingPreamble(t *testing.T) {
	if got := transpileMode("", true); !strings.HasPrefix(got, "#define SAWIT_FREESTANDING\n") {
		t.Errorf("freestanding output harus diawali define, got:\n%s", got)
	}
	if got := transpile(""); strings.Contains(got, "SAWIT_FREESTANDING") {
		t.Errorf("mode hosted tidak boleh menyisipkan define freestanding:\n%s", got)
	}
}
