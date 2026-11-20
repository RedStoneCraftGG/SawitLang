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
}

func TestFreestandingPreamble(t *testing.T) {
	if got := transpileMode("", true); !strings.HasPrefix(got, "#define SAWIT_FREESTANDING\n") {
		t.Errorf("freestanding output harus diawali define, got:\n%s", got)
	}
	if got := transpile(""); strings.Contains(got, "SAWIT_FREESTANDING") {
		t.Errorf("mode hosted tidak boleh menyisipkan define freestanding:\n%s", got)
	}
}
