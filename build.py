"""Reproducible native GBC ROM build with GBDK 2020 (no cartridge save RAM)."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess
from scripts.ui_assets import generate
ROOT=Path(__file__).resolve().parent
def assets():
    glyphs=json.loads((ROOT/'assets/font.json').read_text())
    logo=json.loads((ROOT/'assets/logo.json').read_text())
    ascii_map=[0]*128;cyrillic=[0]*33
    for i,g in enumerate(glyphs):
        c=g['code']
        if c<128:ascii_map[c]=i
        elif 0x410<=c<=0x42f:cyrillic[c-0x410]=i
        elif c==0x401:cyrillic[32]=i
    raw=[int(row[x:x+8],2) for row in logo['rows'] for x in range(0,56,8)]
    lines=['/* Generated from committed font/logo JSON. */','#ifndef ASSETS_H','#define ASSETS_H']
    for name,data in [('font_ascii',ascii_map),('font_cyrillic',cyrillic),('logo_mask',raw)]:
        lines.append(f'static const unsigned char {name}[{len(data)}] = {{'+','.join(map(str,data))+'};')
    lines.append('static const unsigned char font_rows[][7] = {'+','.join('{'+','.join(map(str,g['rows']))+'}' for g in glyphs)+'};')
    lines.append('#endif')
    (ROOT/'build/assets.h').write_text('\n'.join(lines)+'\n',encoding='ascii')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--gbdk');args=ap.parse_args()
    candidate=args.gbdk or os.environ.get('GBDK_HOME')
    lcc=Path(candidate)/'bin/lcc.exe' if candidate else ROOT/'tools/gbdk/bin/lcc.exe'
    if not lcc.exists():
        found=shutil.which('lcc')
        if not found:raise SystemExit('Install GBDK 4.5.0 in tools/gbdk, set GBDK_HOME, or pass --gbdk PATH.')
        lcc=Path(found)
    (ROOT/'build').mkdir(exist_ok=True);(ROOT/'dist').mkdir(exist_ok=True);assets();generate(ROOT)
    command=[str(lcc),'-msm83:gb','-Wl-m','-Wl-j','-Wm-yt0','-Wm-yo2','-Wm-ynPLAYONGBC','-Wm-yC','-Wl-g.STACK=0xcfff','-Wf--opt-code-speed','-Isrc','-Ibuild','-o','build/play-on-gbc.gbc']
    command += [str(p.relative_to(ROOT)) for p in sorted((ROOT/'src').glob('*.c'))]
    subprocess.run(command,cwd=ROOT,check=True)
    rom=(ROOT/'build/play-on-gbc.gbc').read_bytes()
    assert len(rom)==32768 and rom[0x143]==0xc0 and rom[0x147]==0 and rom[0x149]==0
    assert rom[0x48]==0xc3 and rom[0x58]==0xc3,'Interrupt vectors were shifted by the linker'
    check=0
    for b in rom[0x134:0x14d]:check=(check-b-1)&255
    assert check==rom[0x14d]
    # Keep compiler-managed data away from the explicitly reserved RAM test and stack.
    symbols=(ROOT/'build/play-on-gbc.noi').read_text()
    match=re.search(r'DEF s__DATA (0x[0-9A-Fa-f]+)',symbols)
    length=re.search(r'DEF l__DATA (0x[0-9A-Fa-f]+)',symbols)
    if match and length:assert int(match[1],16)+int(length[1],16)<=0xce00,'Global data overlaps fixed stack reserve'
    assert re.search(r'DEF \.STACK 0xCFFF\b',symbols),'Stack must stay in fixed WRAM'
    (ROOT/'dist/play-on-gbc.gbc').write_bytes(rom)
    sha=hashlib.sha256(rom).hexdigest()
    (ROOT/'dist/SHA256SUMS.txt').write_text(f'{sha}  play-on-gbc.gbc\n',encoding='ascii')
    print(f'ROM: dist/play-on-gbc.gbc ({len(rom)} bytes)\nSHA256: {sha}')
if __name__=='__main__':main()
