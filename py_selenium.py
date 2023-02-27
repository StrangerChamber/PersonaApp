from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import time
import pandas as pd
import random
import logging
import os
#from sys import exit

# this is only needed when in use with a different program so it can be terminated remotely
with open('/home/kali/selenium_pid.txt', 'w', encoding='utf-8') as f:
    f.write(str(os.getpid()))

# creating a log file
logging.basicConfig(filename='/home/kali/selenium_file.log', filemode='w+')

# a list of URLs that will be visited
urls = ['https://www.youtube.com','https://gmail.com','https://trends.google.com/trends/explore?q=lstm','https://google.com']

# see here for RPi setup instructions: https://docs.google.com/document/d/1f3qmZxbf5u6bzq5np6h2j_45iWju4ydApB0IWMoKJZQ/edit?usp=sharing
# for other Debian based platforms: sudo apt install chromium-driver
op = webdriver.ChromeOptions()
op.add_argument('--headless')
op.add_argument('--no-sandbox')
#op.add_argument('--disable-dev-shm-usage')

# the options below are necessary to run selenium on a headless system
# change the download path to something valid on your machine
download_path = '/home/kali/data'
# change the default download directory
download_prefs={'download.default_directory': r"/home/kali/data", "download.prompt_for_download": False}
op.add_experimental_option("prefs", download_prefs)
op.add_argument('--window-size=1920,1080')
op.add_argument('--ignore-certificate-errors')
op.add_argument('--allow-running-insecure-content')
op.add_argument('--disable-blink-features=AutomationControlled')

# without setting the user agent, google trends blocks access
user_agent = 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.50 Safari/537.36'
op.add_argument(f'user-agent={user_agent}') 

"""
def enable_headless_download(browser, download_path):
    browser.command_executor._commands["send_command"] = ("POST", '/session/$sessionId/chromium/send_command')
    params = {'cmd': 'Page.setDownloadBehavior', 'params':{'behavior':'allow', 'downloadPath': download_path}}
    command_result = browser.execute("send_command", params)
"""

# point to the location of "chromedriver" --> open terminal and execute "which chromedriver"
with webdriver.Chrome("/usr/bin/chromedriver", options=op) as driver:
    #driver.Chrome(executable_path='/home/kali/bin/chromedriver')
    
    #enable headless downloads
    #enable_headless_download(driver, download_path)
    
    # Setup wait for later
    wait = WebDriverWait(driver, 10)
    
    # First, go to youtube, search for a mix, and start it
    try:
        logging.info("Browser opened")
        print("Browser opened")
        driver.get(urls[0])
        logging.info(driver.title) 

        # Store the ID of the original window
        original_window = driver.current_window_handle

        # Check we don't have other windows open already
        assert len(driver.window_handles) == 1

        # do nothing until the element has been detected
        search = wait.until(
            EC.presence_of_element_located((By.NAME, "search_query"))
        )
        # then click on the element
        search.click()

        # and enter the text below
        search.send_keys("pop mix")
        search.submit()

        links = wait.until(
            EC.presence_of_all_elements_located((By.ID, "video-title"))
        )
        #print(links)
    
        links[1].click()
        logging.info(driver.title)
        print(driver.title)
        time.sleep(10)
    except Exception as e:
        logging.error("error accessing youtube: %s"%(repr(e)))
        print("error accessing youtube: %s"%(repr(e)))
        pass
        
    try:
        # open a new tab
        driver.execute_script("window.open('');")
        driver.switch_to.window(driver.window_handles[1])
        # go to gmail
        driver.get(urls[1])
        logging.info(driver.title)
        print(driver.title)
        time.sleep(5)

        # nothing else is done with gmail, however entering username and password could be configured
    except Exception as e:
        logging.error("error opening gmail: %s"%(repr(e)))
        print("error opening gmail: %s"%(repr(e)))
        pass
        
    try:    
        # new tab again
        driver.execute_script("window.open('');")
        driver.switch_to.window(driver.window_handles[2])
        # open google trends
        driver.get(urls[2])
        logging.info(driver.title)
        print(driver.title)
        time.sleep(10)

        # this downloads a csv with the top searches related to 'lstm' (long short-term memory)
        exports = driver.find_elements(By.CLASS_NAME, "export")
        # the screenshot can be removed
        # this was used as a way of confirming that the script works in headless mode
        # an empty white page will be saved if access was blocked by google trends
        driver.get_screenshot_as_file("/home/kali/export_screenshot.png")
        #print(exports)

        exports[2].click()
        time.sleep(5)
    except Exception as e:
        logging.error("error getting google trends csv: %s"%(repr(e)))
        print("error getting google trends csv: %s"%(repr(e)))
        pass
        
    # load the searches into a dataframe
    df = pd.read_csv('/home/kali/data/relatedEntities.csv', skiprows=4, names=["word","count"])
    # some cleanup
    df = df[df["word"].str.contains('RISING')==False]
    searches = df[df.columns[0]].dropna()
    searches = searches.tolist()
    
    # this loop will run forever
    while True:
        try:
            # random number for grabbing a random search term from the dataframe
            search_number = random.randrange(len(searches))

            # go to google
            driver.get(urls[3])
            time.sleep(5)
            # find the search field and submit the search
            search_field = driver.find_element(By.NAME, 'q')
            search_field.send_keys(searches[search_number])
            search_field.send_keys(Keys.RETURN)
            time.sleep(5)
            logging.info(driver.title)
            print(driver.title)

            # find the first result and click on it
            result = wait.until(
                EC.element_to_be_clickable((By.XPATH, "//div[@class='yuRUbf']/a"))
            ) 

            result.click()
            time.sleep(5)
            logging.info(driver.title)
            print(driver.title)
            time.sleep(random.randint(20,40))
        except Exception as e:
            logging.error("error searching on google.com: %s"%(repr(e)))
            print("error searching on google.com: %s"%(repr(e)))
            # uncomment below if you want the script to end when an error occurs
            # else, since errors can occurr when clicking on google's first result
            # just move on to the next
            #driver.quit()
            #exit()
            pass
