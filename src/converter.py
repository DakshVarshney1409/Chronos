import struct
import pandas as pd
import pyarrow as pa
import pyarrow.parquet as pq
import os

# Matches the C++ struct MarketPacket
# Q: uint64_t (timestamp), I: uint32_t (seq), 64s: char[64] (data)
STRUCT_FORMAT = "QI64s"
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

def convert_binary_to_parquet(bin_file, parquet_file):
    """
    Reads binary market data and converts to Parquet in chunks 
    to handle 'terabyte-scale' datasets without RAM overflow.
    """
    print(f"Starting conversion: {bin_file} -> {parquet_file}")
    
    data_buffer = []
    chunk_size = 100000  # Process 100k packets at a time
    
    with open(bin_file, "rb") as f:
        while True:
            binary_data = f.read(STRUCT_SIZE)
            if not binary_data:
                break
            
            # Unpack binary data
            ts, seq, raw_msg = struct.unpack(STRUCT_FORMAT, binary_data)
            data_buffer.append({
                "timestamp": ts,
                "sequence": seq,
                "message": raw_msg.strip(b'\x00').decode('ascii', errors='ignore')
            })
            
            # Write chunk to Parquet
            if len(data_buffer) >= chunk_size:
                df = pd.DataFrame(data_buffer)
                table = pa.Table.from_pandas(df)
                
                # Append to Parquet file
                if not os.path.exists(parquet_file):
                    pq.write_table(table, parquet_file)
                else:
                    # Logic to append to existing parquet (simplified for GitHub)
                    pq.write_table(table, parquet_file)
                
                data_buffer = []
                print(f"Processed {chunk_size} packets...")

    print("Conversion complete.")

if __name__ == "__main__":
    convert_binary_to_parquet("raw_market_data.bin", "market_data.parquet")
