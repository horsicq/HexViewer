"""
X86/X64 Disassembler Plugin for HexViewer
Uses Capstone disassembly engine
"""

try:
    from capstone import *
except ImportError:
    print("ERROR: Capstone not installed. Install with: pip install capstone")
    raise

def disassemble(data, offset=0, arch="x64", max_instructions=1):
    """
    Disassemble binary data using Capstone
    
    Args:
        data: bytes - The binary data to disassemble
        offset: int - Starting offset in the file
        arch: str - Architecture ("x86", "x64", "arm", "arm64", "mips")
        max_instructions: int - Maximum number of instructions to disassemble
    
    Returns:
        list of dict with keys: 'address', 'bytes', 'mnemonic', 'operands', 'size'
    """
    
    # Map architecture strings to Capstone constants
    arch_map = {
        "x86": (CS_ARCH_X86, CS_MODE_32),
        "x64": (CS_ARCH_X86, CS_MODE_64),
        "arm": (CS_ARCH_ARM, CS_MODE_ARM),
        "arm64": (CS_ARCH_ARM64, CS_MODE_ARM),
        "thumb": (CS_ARCH_ARM, CS_MODE_THUMB),
        "mips": (CS_ARCH_MIPS, CS_MODE_MIPS32),
        "mips64": (CS_ARCH_MIPS, CS_MODE_MIPS64),
    }
    
    # Default to x64 if unknown architecture
    if arch.lower() not in arch_map:
        arch = "x64"
    
    cs_arch, cs_mode = arch_map[arch.lower()]
    
    # Create Capstone disassembler instance
    try:
        md = Cs(cs_arch, cs_mode)
        md.detail = True  # Enable detailed instruction info
    except CsError as e:
        return [{
            'address': offset,
            'bytes': '',
            'mnemonic': 'error',
            'operands': str(e),
            'size': 1
        }]
    
    results = []
    
    try:
        # Disassemble the data
        count = 0
        for insn in md.disasm(data, offset):
            if count >= max_instructions:
                break
            
            # Format instruction bytes as hex string
            byte_str = ' '.join(f'{b:02X}' for b in insn.bytes)
            
            results.append({
                'address': insn.address,
                'bytes': byte_str,
                'mnemonic': insn.mnemonic,
                'operands': insn.op_str,
                'size': insn.size
            })
            
            count += 1
        
        # If no instructions were disassembled, return invalid instruction
        if not results:
            results.append({
                'address': offset,
                'bytes': data[0:1].hex().upper() if data else '',
                'mnemonic': 'db',
                'operands': f'0x{data[0]:02X}' if data else '0x00',
                'size': 1
            })
    
    except CsError as e:
        # Handle disassembly errors
        results.append({
            'address': offset,
            'bytes': data[0:1].hex().upper() if data else '',
            'mnemonic': 'error',
            'operands': str(e),
            'size': 1
        })
    
    return results


def get_info():
    """
    Returns plugin metadata
    """
    return {
        'name': 'X86/X64 Disassembler',
        'version': '1.0',
        'author': 'HexViewer',
        'description': 'Disassembles x86/x64 machine code using Capstone',
        'architectures': ['x86', 'x64', 'arm', 'arm64', 'thumb', 'mips', 'mips64']
    }


# Test function for standalone usage
if __name__ == "__main__":
    # Example: Disassemble some x64 code
    # mov rax, 0x1234567890ABCDEF
    # ret
    test_code = bytes([
        0x48, 0xB8, 0xEF, 0xCD, 0xAB, 0x90, 0x78, 0x56, 0x34, 0x12,
        0xC3
    ])
    
    print("Testing X86/X64 Disassembler Plugin")
    print("=" * 50)
    
    instructions = disassemble(test_code, offset=0x1000, arch="x64", max_instructions=10)
    
    for insn in instructions:
        addr = insn['address']
        bytes_str = insn['bytes']
        mnem = insn['mnemonic']
        ops = insn['operands']
        
        print(f"0x{addr:08X}:  {bytes_str:24s}  {mnem:8s} {ops}")