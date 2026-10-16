{
  description = "Project Flake";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };
  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      pkgsCross = import pkgs.path {
        inherit system;
        crossSystem = {
          config = "x86_64-w64-mingw32";
        };
      };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages =
          with pkgs;
          [
            pkg-config
            clang-tools
            gcc
            gnumake
            cmake
            shaderc

            libx11
            libxrandr
            libxinerama
            libxcursor
            libxi
            libxext
            wayland
            wayland-scanner
            egl-wayland
            libxkbcommon

            wineWow64Packages.stable
          ]
          ++ [
            pkgsCross.stdenv.cc
          ];

        # Formatter decided it looks good this way.
        shellHook = ''
          			export WINEPREFIX=$(pwd)/.wine
          			'';
      };
    };
}
