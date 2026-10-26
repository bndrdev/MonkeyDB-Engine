use std::env;
use std::path::PathBuf;
fn main() {
    let dir = env::var_os("MONKEYDB_LIB_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../build"));
    println!("cargo:rustc-link-search=native={}", dir.display());
    println!("cargo:rerun-if-env-changed=MONKEYDB_LIB_DIR");
}
