#!/usr/bin/env python3
"""Check artwork preservation through real staging code in temporary trees."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('picori_build', ROOT / 'build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


class PackagingTest(unittest.TestCase):
    def test_selected_region_rom_is_prepared_without_overwriting_another(self):
        with tempfile.TemporaryDirectory(prefix='tmc-art-rom-') as tmp:
            root = Path(tmp)
            write(root / 'baserom_eu.gba', b'europe')
            write(root / 'baserom.gba', b'usa')
            with mock.patch.object(build, 'REPO_ROOT', root):
                self.assertTrue(build._prepare_developer_rom('EU'))
                output = root / 'build/pc/baserom.gba'
                self.assertEqual(output.read_bytes(), b'europe')
                self.assertTrue(build._prepare_developer_rom('EU'))
                self.assertFalse(build._prepare_developer_rom('USA'))
                self.assertEqual(output.read_bytes(), b'europe')

    def test_full_staging_preserves_custom_art_mods_and_region_caches(self):
        with tempfile.TemporaryDirectory(prefix='tmc-art-stage-') as tmp:
            root = Path(tmp)
            dist = root / 'dist' / 'USA'
            asset = dist / 'assets' / 'override' / '169' / 'menu_titled_background_normal.png'
            mod = dist / 'mods' / 'sample' / 'gfx' / 'sprite.bin'
            region = dist / 'assets' / 'eu' / 'cached.bin'
            edit = dist / 'assets_src' / 'eu' / 'cached.json'
            write(asset, b'artist custom bytes')
            write(mod, b'user mod')
            write(region, b'region cache')
            write(edit, b'editable cache')
            write(dist / '.assets_fingerprint', b'unchanged\n')
            write(root / 'build/USA/assets/override/169/menu_titled_background_normal.png', b'packaged default')
            write(root / 'build/USA/assets/override/219/another.png', b'new file')
            write(root / 'build/USA/assets/generated.bin', b'new generated data')
            write(root / 'build/USA/assets_src/generated.json', b'new editable data')
            with mock.patch.object(build, 'REPO_ROOT', root):
                build._invalidate_region_caches(dist, 'unchanged')
                build._stage_dist_asset_trees(dist, 'USA')
                self.assertEqual(asset.read_bytes(), b'artist custom bytes')
                self.assertEqual((dist / 'assets/override/219/another.png').read_bytes(), b'new file')
                self.assertEqual(mod.read_bytes(), b'user mod')
                self.assertEqual(region.read_bytes(), b'region cache')
                self.assertEqual(edit.read_bytes(), b'editable cache')
                self.assertEqual((dist / 'assets/generated.bin').read_bytes(), b'new generated data')
                build._invalidate_region_caches(dist, 'changed')
                self.assertFalse(region.exists())
                self.assertFalse(edit.exists())
                self.assertEqual(asset.read_bytes(), b'artist custom bytes')
                self.assertEqual(mod.read_bytes(), b'user mod')

    def test_failed_generated_copy_restores_artwork(self):
        with tempfile.TemporaryDirectory(prefix='tmc-art-stage-failure-') as tmp:
            root = Path(tmp)
            dist = root / 'dist/USA'
            art = dist / 'assets/override/43/title_screen.png'
            write(art, b'irreplaceable art')
            source = root / 'build/USA/assets'
            write(source / 'generated.bin', b'generated')
            original = shutil.copytree

            def fail_generated(src, dst, *args, **kwargs):
                if Path(src) == source:
                    raise OSError('injected generated asset copy failure')
                return original(src, dst, *args, **kwargs)

            with mock.patch.object(build, 'REPO_ROOT', root), mock.patch.object(build.shutil, 'copytree', side_effect=fail_generated):
                with self.assertRaises(OSError):
                    build._stage_dist_asset_trees(dist, 'USA')
            self.assertEqual(art.read_bytes(), b'irreplaceable art')

    @unittest.skipUnless(Path('/usr/lib/aarch64-linux-gnu/libxkbcommon.so.0').exists() and
                         Path('/usr/share/doc/libxkbcommon0/copyright').exists(),
                         'RG staging system fixtures are unavailable')
    def test_rg_slim_staging_preserves_art_and_mods(self):
        with tempfile.TemporaryDirectory(prefix='tmc-art-rg-') as tmp:
            root = Path(tmp)
            for name in ('build-rgdsplus.sh', 'launch-rgdsplus.sh'):
                path = root / 'scripts' / name
                path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT / 'scripts' / name, path)
            write(root / 'build/pc/tmc_pc', b'fake binary')
            write(root / 'assets/rando/default.logic', b'logic')
            art = root / 'dist/rgdsplus/assets/override/169/title_screen.png'
            mod = root / 'dist/rgdsplus/mods/user/gfx.bin'
            write(art, b'rg artist art')
            write(mod, b'rg mod')
            bindir = root / 'bin'
            bindir.mkdir()
            for name in ('xmake', 'cmake', 'ninja', 'make', 'pkg-config', 'aarch64-linux-gnu-gcc',
                         'aarch64-linux-gnu-g++', 'aarch64-linux-gnu-strip'):
                path = bindir / name
                path.write_text('#!/bin/sh\nexit 0\n')
                path.chmod(0o755)
            readelf = bindir / 'aarch64-linux-gnu-readelf'
            readelf.write_text('#!/bin/sh\nprintf "Class: ELF64\\nMachine: AArch64\\n"\n')
            readelf.chmod(0o755)
            env = os.environ.copy()
            env['PATH'] = str(bindir) + os.pathsep + env['PATH']
            subprocess.run(['sh', str(root / 'scripts/build-rgdsplus.sh')], cwd=root, env=env,
                           check=True, stdout=subprocess.DEVNULL)
            self.assertEqual(art.read_bytes(), b'rg artist art')
            self.assertEqual(mod.read_bytes(), b'rg mod')


if __name__ == '__main__':
    unittest.main()
