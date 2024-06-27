{ lib
, pkgs
, stdenv
, scons
, godot-cpp
, withPlatform ? "linux"
, withTarget ? "template_release"
}:

let
  mkSconsFlagsFromAttrSet = lib.mapAttrsToList (k: v:
    if builtins.isString v
    then "${k}=${v}"
    else "${k}=${builtins.toJSON v}");
in
stdenv.mkDerivation {
  name = "godot-extension-unix-socket";
  src = ./.;

  nativeBuildInputs = with pkgs; [ scons pkg-config makeWrapper ];
  buildInputs = with pkgs; [ godot-cpp ];
  enableParallelBuilding = true;
  BUILD_NAME = "nix-flake";

  sconsFlags = mkSconsFlagsFromAttrSet {
    platform = withPlatform;
    target = withTarget;
  };

  outputs = [ "out" ];

  preBuild = ''
    substituteInPlace SConstruct \
    --replace-fail 'godot-cpp/SConstruct' '${godot-cpp}/SConstruct'
  '';

  installPhase = ''
    mkdir -p "$out/bin"
    cp addons/unixsock/bin/*.so $out/bin/
    cp addons/unixsock/unixsock.gdextension $out/
  '';

  meta = with lib; {
    homepage = "https://github.com/mindwm/gdextension-unix-socket";
    description = "A Godot extension to communicate via Unix sockets";
    license = licenses.gpl3;
    platforms = [ "x86_64-linux" "aarch64-linux" ];
    maintainers = with maintainers; [ omgbebebe ];
  };
}
