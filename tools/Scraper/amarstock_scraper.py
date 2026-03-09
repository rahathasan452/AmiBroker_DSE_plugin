
# %%
# Add the code you want to run separately here
import requests
import pandas as pd
from datetime import datetime, timedelta
import time
import io

# %%
# The target URL for downloading Amar Stock CSV data.
DOWNLOAD_URL = 'https://www.amarstock.com/data/download/CSV' # change to exact form action URL if different





# %%
def fetch_dataframe_for_date(target_date, data_type='adjusted'):
    '''
    Fetches the CSV data for the given target_date (YYYY-MM-DD) and returns it as a Pandas DataFrame.
    Returns None if failed.
    '''
    print(f'Attempting to fetch data for {target_date}...')
    
    # Payload matching the form inputs
    payload = {
        'date': target_date,
        'type': data_type # assuming type is adjusted or unadjusted based on the radio button
    }
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36',
        'Referer': 'https://amarstock.com/'
    }
    
    try:
        response = requests.post(DOWNLOAD_URL, data=payload, headers=headers)
        
        # Check if response is successful and contains data
        if response.status_code == 200 and len(response.text) > 100:
            # Extremely fast string-level filtering before pandas even touches the data
            lines = response.text.splitlines()
            if not lines: return None
            
            target_indices = {'00DS30', '00DSES', '00DSEX', '00DSMEX'}
            
            # Keep header and only rows where the second column (Scrip) is in target_indices
            filtered_lines = [lines[0]]
            for line in lines[1:]:
                parts = line.split(',')
                if len(parts) > 1 and parts[1].strip() in target_indices:
                    filtered_lines.append(line)
                    
            # Load the natively filtered tiny dataset (~4 rows) into Pandas directly
            csv_data = io.StringIO('\n'.join(filtered_lines))
            df = pd.read_csv(csv_data)
                
            return df
        else:
            print(f'Failed or no data found for {target_date}. Status Code: {response.status_code}')
            return None
    except Exception as e:
        print(f'Error fetching data for {target_date}: {e}')
        return None




# %%
# Get today's data straight into memory
today_date = datetime.now().strftime('%Y-%m-%d')
print("Fetching today's data...")
df_today = fetch_dataframe_for_date(today_date)

if df_today is not None:
    print(df_today.head())






# %%
start_date_str = '2025-01-01'  # Change to your 'from' date
end_date_str = '2026-02-15'    # Change to your 'to' date

start_date = datetime.strptime(start_date_str, '%Y-%m-%d')
end_date = datetime.strptime(end_date_str, '%Y-%m-%d')

all_dataframes = []

current_date = start_date
while current_date <= end_date:
    # Skip Friday (4) and Saturday (5) as the market is closed
    #if current_date.weekday() not in [4, 5]:
        cur_date_str = current_date.strftime('%Y-%m-%d')
        df_day = fetch_dataframe_for_date(cur_date_str)
        
        if df_day is not None:
            # Add a Date column just in case the data doesn't have it natively
            if 'Date' not in df_day.columns and 'DATE' not in df_day.columns:
                df_day['Date'] = cur_date_str
            all_dataframes.append(df_day)
                
        # Sleep to avoid overwhelming the server
        time.sleep(0.000000000000000000001)
    
        current_date += timedelta(days=1)

# Merge everything
if all_dataframes:
    merged_df = pd.concat(all_dataframes, ignore_index=True)
    
    print(f'\nSuccessfully merged {len(all_dataframes)} days of data!')
    print(f'Total merged rows: {len(merged_df)}')
else:
    print('No DataFrames could be fetched.')








# %%
# Preview the merged dataframe
if all_dataframes:
    print(merged_df.head())
    print(merged_df.tail())

    # Standardize column mappings mapping
    col_mapping = {}
    for col in merged_df.columns:
        c_up = col.upper().strip()
        if c_up in ['DATE', 'DATE_YMD']: col_mapping[col] = 'Date_YMD'
        elif c_up in ['SCRIP', 'TRADING_CODE', 'TICKER']: col_mapping[col] = 'Ticker'
        elif c_up == 'OPEN': col_mapping[col] = 'Open'
        elif c_up == 'HIGH': col_mapping[col] = 'High'
        elif c_up == 'LOW': col_mapping[col] = 'Low'
        elif c_up == 'CLOSE': col_mapping[col] = 'Close'
        elif c_up == 'VOLUME': col_mapping[col] = 'Volume'
        
    merged_df = merged_df.rename(columns=col_mapping)
    
    # Format Date to use hyphens (YYYY-MM-DD)
    if 'Date_YMD' in merged_df.columns:
        def format_date(d):
            d_str = str(d).strip()
            # If it comes as YYYYMMDD without hyphens
            if len(d_str) == 8 and d_str.isdigit():
                return f"{d_str[:4]}-{d_str[4:6]}-{d_str[6:]}"
            return d_str
        merged_df['Date_YMD'] = merged_df['Date_YMD'].apply(format_date)
        
    required_cols = ['Ticker', 'Date_YMD', 'Open', 'High', 'Low', 'Close', 'Volume']
    final_cols = [c for c in required_cols if c in merged_df.columns]
    
    # Export single ticker files without the Ticker column
    save_cols = [c for c in final_cols if c != 'Ticker']
    for ticker in ['00DS30', '00DSES', '00DSEX', '00DSMEX']:
        if 'Ticker' in merged_df.columns:
            ticker_df = merged_df[merged_df['Ticker'] == ticker]
            if not ticker_df.empty:
                ticker_df[save_cols].to_csv(f"{ticker}.csv", index=False)
                print(f"Saved {ticker}.csv with {len(ticker_df)} records.")

