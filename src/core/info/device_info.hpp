#pragma once



void cpuID(unsigned i, unsigned regs[4]) {
#ifdef _WIN32
  __cpuid((int *)regs, (int)i);

#else
  asm volatile
    ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
     : "a" (i), "c" (0));
  // ECX ��������������� � ���� ��� ������� CPUID 4
#endif
}


int main_cpu_info() {
  unsigned regs[4];

  // ��������� �������������
  char vendor[12];
  cpuID(0, regs);
  ((unsigned *)vendor)[0] = regs[1]; // EBX
  ((unsigned *)vendor)[1] = regs[3]; // EDX
  ((unsigned *)vendor)[2] = regs[2]; // ECX
  std::string cpuVendor = std::string(vendor, 12);

  // ��������� ������������� ����������
  cpuID(1, regs);
  unsigned cpuFeatures = regs[3]; // EDX

  // ���������� ���������� ���� �� ����������
  cpuID(1, regs);
  unsigned logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
  std::cout << " ���������� ����: " << logical << std::endl;
  unsigned cores = logical;

  if (cpuVendor == "GenuineIntel") {
    // ��������� ���������� � ����������� DCP
    cpuID(4, regs);
    cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1

  } else if (cpuVendor == "AuthenticAMD") {
    // Get NC: Number of CPU cores - 1
    cpuID(0x80000008, regs);
    cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
  }

  std::cout << "    ���� ����������: " << cores << std::endl;

  // ����������� ���������������
  bool hyperThreads = cpuFeatures & (1 << 28) && cores < logical;

  std::cout << "���������������: " << (hyperThreads ? "true" : "false") << std::endl;

  return 0;
}

