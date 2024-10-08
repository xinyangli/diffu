{ lib
, stdenv
, cmake
, mini-gdbstub
, cli11
, spdlog
}: stdenv.mkDerivation {
  pname = "diffu";
  version = "0.0.0";

  src = ./.;

  nativeBuildInputs = [
    cmake
    mini-gdbstub
    cli11
    spdlog
  ];
}
