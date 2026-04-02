#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if defined(__linux__)
  #if defined(__aarch64__)
    #include <asm/hwcap.h>
    #include <sys/auxv.h>
  #elif defined(__x86_64__) || defined(__i386__)
    #include <cpuid.h>
  #endif
#elif defined(__APPLE__)
  #include <sys/sysctl.h>

static int sysctlbynameErr(const char *name, void *oldp, size_t *oldlenp,
                           void *newp, size_t newlen) {
  int err = sysctlbyname(name, oldp, oldlenp, newp, newlen);
  if (err == -1) {
    err = errno;
    if (err != ENOENT) {
      fprintf(stderr, "*** ERR sysctlbyname(\"%s\") => errno:%d strerror:%s\n",
              name, err, strerror(err));
      exit(EXIT_FAILURE);
    }
  }
  return err;
}
#endif

//---------------------------------------------------------------------
//
// main
//

int main(int argc __attribute__((unused)),
         const char *argv[] __attribute__((unused))) {
#if defined(__linux__)
  const char *os = "Linux";
  #if defined(__x86_64__) || defined(__i386__)
  //-------------------------------------------------------------------
  // Linux x86.

  const char *arch = "x86";
  bool hasNativeSHA1 = false;
  bool hasNativeAES = false;

  // Check if leaf 7 is available
  if (__get_cpuid_max(7, NULL) >= 7) {
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    hasNativeSHA1 = (ecx & (1 << 1)) != 0;
  }

  // AES-NI is indicated by CPUID leaf 1, ECX bit 25
  {
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
      hasNativeAES = (ecx & (1 << 25)) != 0;
    }
  }
  #elif defined(__aarch64__)
  //-------------------------------------------------------------------
  // Linux arm64.

  const char *arch = "arm64";
  unsigned long hwcaps = getauxval(AT_HWCAP);
  bool hasNativeSHA1 = (hwcaps & HWCAP_SHA1) != 0;
  bool hasNativeAES = (hwcaps & HWCAP_AES) != 0;
  #else
    #error Currently-Unsupported Linux ISA
  #endif
#elif defined(__APPLE__)
  #if defined(__aarch64__)
  //-------------------------------------------------------------------
  // Apple Silicon.

  const char *os = "Apple"; // macOS, iOS, tvOS, watchOS, visionOS, etc.
  const char *arch = "arm64";

  int supported = 0;
  size_t len = sizeof(supported);
  int err =
      sysctlbynameErr("hw.optional.arm.FEAT_SHA1", &supported, &len, NULL, 0);
  bool hasNativeSHA1 = !err && supported;

  supported = 0;
  len = sizeof(supported);
  err = sysctlbynameErr("hw.optional.arm.FEAT_AES", &supported, &len, NULL, 0);
  bool hasNativeAES = !err && supported;
  #elif defined(__x86_64__)
  //-------------------------------------------------------------------
  // Apple macOS Intel.

  const char *os = "macOS";
  const char *arch = "Intel";

  bool hasNativeSHA1 = false;
  bool hasNativeAES = false;

  char features[1024];
  memset(features, 0, sizeof(features));
  size_t size = sizeof(features);
  int err =
      sysctlbynameErr("machdep.cpu.leaf7_features", features, &size, NULL, 0);
  hasNativeSHA1 = !err && strstr(features, "SHA") != NULL;

  if (!err) {
    memset(features, 0, sizeof(features));
    size = sizeof(features);
    err = sysctlbynameErr("machdep.cpu.features", features, &size, NULL, 0);
    hasNativeAES = !err && strstr(features, "AES") != NULL;
  }
  #else
    #error Currently-Unsupported Apple ISA
  #endif
#endif

  printf("{\n"
         "  \"arch\": \"%s\",\n"
         "  \"os\": \"%s\",\n"
         "  \"hasNativeAES\": %s,\n"
         "  \"hasNativeSHA1\": %s,\n"
         "}\n",
         arch, os, hasNativeAES ? "true" : "false",
         hasNativeSHA1 ? "true" : "false");

  return EXIT_SUCCESS;
}
