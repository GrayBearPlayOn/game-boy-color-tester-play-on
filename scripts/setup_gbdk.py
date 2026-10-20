"""Install the pinned official Windows x64 GBDK build locally, with SHA256 check."""
from pathlib import Path
import hashlib,urllib.request,zipfile
ROOT=Path(__file__).resolve().parents[1]
URL='https://github.com/gbdk-2020/gbdk-2020/releases/download/4.5.0/gbdk-win64.zip'
SHA256='266854ce92e3064871c5b28cd3436cc2a6cb136af9e7cf617140108f8c1c5890'
tools=ROOT/'tools';tools.mkdir(exist_ok=True)
archive=tools/'gbdk-win64.zip'
if not archive.exists():urllib.request.urlretrieve(URL,archive)
if hashlib.sha256(archive.read_bytes()).hexdigest()!=SHA256:raise SystemExit('GBDK archive checksum mismatch.')
with zipfile.ZipFile(archive) as z:
    for name in z.namelist():
        target=(tools/name).resolve()
        if not target.is_relative_to(tools.resolve()):raise SystemExit('Unsafe archive member')
    z.extractall(tools)
print('GBDK 4.5.0 installed:',tools/'gbdk')
