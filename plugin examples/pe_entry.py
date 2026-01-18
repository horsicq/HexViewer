def get_info():
    return {
        "name": "PE Entry Point Bookmark",
        "version": "2.0",
        "author": "HexViewer",
        "description": "Bookmarks PE entry point in files and live process memory"
    }

def generate_bookmarks(data: bytes, size: int, memory_map=None):
    """
    Args:
        data: Binary data (file or live process memory)
        size: Size of data
        memory_map: List of memory regions (only for process memory)
    """
    bookmarks = []
    
    if not data or size < 0x100:
        return bookmarks
    
    is_process_memory = memory_map and len(memory_map) > 0
    
    # PE base is always at offset 0 in our buffer
    pe_base_offset = 0
    
    # Check MZ header
    if data[0:2] != b"MZ":
        return bookmarks
    
    # Get PE header offset (e_lfanew at offset 0x3C)
    if size < 0x40:
        return bookmarks
    
    pe_offset = int.from_bytes(data[0x3C:0x40], "little")
    
    if pe_offset + 0x100 > size:
        return bookmarks
    
    # Check PE signature
    if data[pe_offset:pe_offset + 4] != b"PE\x00\x00":
        return bookmarks
    
    # Optional header at PE + 24 (after signature + COFF header)
    opt_header_offset = pe_offset + 24
    
    if opt_header_offset + 0x28 > size:
        return bookmarks
    
    # Get magic to determine PE32 vs PE32+
    magic = int.from_bytes(data[opt_header_offset:opt_header_offset + 2], "little")
    is_pe32_plus = (magic == 0x20B)
    
    # Get ImageBase
    image_base_offset = opt_header_offset + (0x18 if is_pe32_plus else 0x1C)
    image_base_size = 8 if is_pe32_plus else 4
    
    if image_base_offset + image_base_size > size:
        return bookmarks
    
    image_base = int.from_bytes(data[image_base_offset:image_base_offset + image_base_size], "little")
    
    # Get AddressOfEntryPoint (offset 16 in Optional Header)
    entry_rva_offset = opt_header_offset + 16
    
    if entry_rva_offset + 4 > size:
        return bookmarks
    
    entry_rva = int.from_bytes(data[entry_rva_offset:entry_rva_offset + 4], "little")
    
    if entry_rva == 0:
        return bookmarks
    
    # Calculate entry point offset
    if is_process_memory:
        # PROCESS MEMORY: RVA maps directly to buffer offset
        # Since we captured the module sequentially, we need to find which
        # region contains the RVA and calculate the buffer offset
        
        entry_va = image_base + entry_rva
        entry_offset = _va_to_buffer_offset(entry_va, memory_map)
        
        if entry_offset is None or entry_offset >= size:
            # Fallback: try direct RVA (works if memory is continuous)
            entry_offset = entry_rva
            if entry_offset >= size:
                return bookmarks
    else:
        # FILE ON DISK: Convert RVA to file offset using section table
        entry_offset = _rva_to_file_offset(data, size, entry_rva, pe_offset)
        if entry_offset is None:
            return bookmarks
    
    # Validate offset
    if entry_offset < 0 or entry_offset >= size:
        return bookmarks
    
    # Create entry point bookmark
    entry_va = image_base + entry_rva
    
    if is_process_memory:
        bookmarks.append({
            "label": "OEP",
            "offset": entry_offset,
            "description": f"Original Entry Point at 0x{entry_offset:X} (VA: 0x{entry_va:X}, RVA: 0x{entry_rva:X})",
            "color": {"r": 0, "g": 255, "b": 0}
        })
    else:
        bookmarks.append({
            "label": "Entry Point", 
            "offset": entry_offset,
            "description": f"Entry Point at 0x{entry_offset:X} (RVA: 0x{entry_rva:X}, VA: 0x{entry_va:X})",
            "color": {"r": 0, "g": 255, "b": 0}
        })
    
    # DOS Header
    bookmarks.append({
        "label": "DOS Header",
        "offset": 0,
        "description": "MZ DOS Header",
        "color": {"r": 100, "g": 100, "b": 255}
    })
    
    # PE Header
    bookmarks.append({
        "label": "PE Header",
        "offset": pe_offset,
        "description": "PE Signature and Headers",
        "color": {"r": 100, "g": 100, "b": 255}
    })
    
    return bookmarks

def _va_to_buffer_offset(va, memory_map):
    """Convert virtual address to buffer offset using memory map"""
    if not memory_map:
        return None
    
    for region in memory_map:
        region_start = region['virtual_address']
        region_end = region_start + region['size']
        
        if region_start <= va < region_end:
            # Calculate offset within this region
            offset_in_region = va - region_start
            # Add to buffer offset
            return region['buffer_offset'] + offset_in_region
    
    return None

def _rva_to_file_offset(data, size, rva, pe_offset):
    """Convert RVA to file offset (for files on disk only)"""
    
    # Get number of sections
    if pe_offset + 8 > size:
        return None
    
    num_sections = int.from_bytes(data[pe_offset + 6:pe_offset + 8], "little")
    
    # Get optional header size
    if pe_offset + 22 > size:
        return None
    
    opt_header_size = int.from_bytes(data[pe_offset + 20:pe_offset + 22], "little")
    
    # Section table starts after: PE sig (4) + COFF header (20) + optional header
    section_table_offset = pe_offset + 24 + opt_header_size
    
    # Search sections
    for i in range(num_sections):
        sec_offset = section_table_offset + (i * 40)
        
        if sec_offset + 40 > size:
            break
        
        virtual_size = int.from_bytes(data[sec_offset + 8:sec_offset + 12], "little")
        virtual_address = int.from_bytes(data[sec_offset + 12:sec_offset + 16], "little")
        raw_size = int.from_bytes(data[sec_offset + 16:sec_offset + 20], "little")
        raw_offset = int.from_bytes(data[sec_offset + 20:sec_offset + 24], "little")
        
        # Check if RVA is in this section's virtual range
        section_end = virtual_address + virtual_size
        
        if virtual_address <= rva < section_end:
            offset_in_section = rva - virtual_address
            
            # Make sure offset is within raw data
            if offset_in_section >= raw_size:
                return None
            
            file_offset = raw_offset + offset_in_section
            
            if 0 <= file_offset < size:
                return file_offset
            
            return None
    
    # If RVA is in headers (before first section), it's direct
    if rva < section_table_offset and rva < size:
        return rva
    
    return None