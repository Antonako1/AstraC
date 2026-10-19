// Helper header for 15_toolchain_improvements.ac
// Tests #line tracking and file-local ('local') vs global ('static') across #include

local U32 file_private_val = 100;
static U32 cross_file_val = 200;

local U32 helper_private_fn(VOID) {
    return file_private_val;
}

static U32 helper_public_fn(VOID) {
    return helper_private_fn() + cross_file_val;
}
