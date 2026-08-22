# -----------------------------------------------------------------------------
# Common per-target options that are not warnings.
# Kept separate from CompilerWarnings.cmake so each concern stays isolated.
# -----------------------------------------------------------------------------
function(configure_options target)
    target_compile_features(${target} PUBLIC cxx_std_20)
endfunction()
