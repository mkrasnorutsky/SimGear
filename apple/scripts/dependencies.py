"""Fetch checksum-pinned published dependencies; no machine-specific installations."""
from pathlib import Path
import hashlib,json,urllib.request,zipfile,io
ROOT=Path(__file__).resolve().parents[2]
import os
WORK=Path(os.environ.get("APPLE_BUILD_ROOT", str(ROOT))).expanduser().resolve()
def fetch():
    folder=WORK/'out/dependencies';folder.mkdir(parents=True,exist_ok=True)
    for name,pin in json.loads((ROOT/'apple/dependencies.json').read_text()).items():
        archive=folder/(name+'.zip')
        if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest()!=pin['sha256']:
            print('Downloading',pin['url'],flush=True)
            with urllib.request.urlopen(pin['url'],timeout=120) as response:data=response.read()
            assert hashlib.sha256(data).hexdigest()==pin['sha256'],'Checksum mismatch: '+name
            archive.write_bytes(data)
        destination=folder/name
        if not (destination/(name+'.xcframework/Info.plist')).exists():
            destination.mkdir(exist_ok=True)
            with zipfile.ZipFile(archive) as z:
                for member in z.infolist():
                    resolved=(destination/member.filename).resolve()
                    if not resolved.is_relative_to(destination.resolve()):raise ValueError('Unsafe archive path')
                z.extractall(destination)
    return {name:folder/name/(name+'.xcframework') for name in json.loads((ROOT/'apple/dependencies.json').read_text())}
def select(artifact,simulator):
    import plistlib
    info=plistlib.loads((artifact/'Info.plist').read_bytes())
    entry=next(x for x in info['AvailableLibraries'] if (x.get('SupportedPlatformVariant')=='simulator')==simulator)
    return artifact/entry['LibraryIdentifier'],entry
if __name__=='__main__':print(fetch())
