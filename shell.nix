{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  nativeBuildInputs = [pkgs.bison pkgs.zig_0_11];
}
