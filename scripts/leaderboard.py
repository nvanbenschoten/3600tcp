from subprocess import call
from time import sleep
import smtplib
import ConfigParser
import ast

def remove_failure(find, items):
    i = 0
    for fail in items:
        if fail['main'] == find['main'] and fail['sub'] == find['sub']:
            del(items[i])
        else:
            i += 1;
    #return -1
    

def get_leaderboard(results_file):
    # run leaderboard script and write current results to a text file
    with open(results_file, 'w') as f:
        f.truncate()
        ret = call(["ssh", "nat@cs3600tcp.ccs.neu.edu", '/course/cs3600f13/bin/project4/printstats'], stdout=f)
        f.close()
    if ret == 0:
        return True
    else:
        return False
    
def parse_leaderboard(results_file):
    # open results file and read lines to list
    with open(results_file) as f:
        leaderboard = f.readlines();
        f.close()

    i = 0;
    # parse list looking for other people in 1st
    while i < len(leaderboard):
        line = leaderboard[i]
        if "-----" in line:
            test_details = line.replace("----- ", "").replace(" -----", "").replace("\n", "")

        if line[0:2] == "1:": # if we are at the leader
            # get the type of the subtest
            subtest = leaderboard[i-1].replace(":", "")
            # get the leader's username
            leader = line.split()[0].split(":")[1] 
            if leader not in usernames: # if we are not the leader
                # find our rank 
                j = i+1
                while 1:
                    user = leaderboard[j].split()[0].split(":")[1]
                    rank = leaderboard[j].split()[0].split(":")[0]
                    if user in usernames:
                        break
                    j += 1
                # send a text message alert that we are failing if not already sent
                fail = {'main':test_details, 'sub':subtest, 'rank':rank}
                if fail not in test_failures:
                    test_failures.append(fail)
                    textAlert(test_details, subtest, rank)
            else:
                fail = {'main':test_details, 'sub':subtest, 'rank':1}
                remove_failure(fail, test_failures)



        i += 1;

def textAlert(test, subtest, rank):
    server = smtplib.SMTP("smtp.gmail.com", 587)
    server.ehlo()
    server.starttls()
    server.login(gmail_info['username'], gmail_info['password'])
    message = "You are currently ranked %d for %s - %s" % (int(rank), test, subtest)
    for a in phonenum_emails:
        body = '\r\n'.join(['To: %s' % a,
                            'From: %s' % gmail_info['username'],
                            'Subject: 3600tcp Alert',
                            '', message])
        server.sendmail(gmail_info['username'], a, body)
    server.quit()

# read in sensitive info from settings file
config_file = 'settings.ini'
configp = ConfigParser.RawConfigParser()
configp.read(config_file)
gmail_info = ast.literal_eval(configp.get('leaderboard', 'gmailinfo'))
phonenum_emails = configp.get('leaderboard', 'phonenumemails').split(',')

# setup other useful things
results = "current_results.txt"
usernames = ["nvanben", "nat"]
test_failures = []
wait_minutes = 3;

# main loop to check leaderboard
print("Monitoring leaderboard")
while 1:
    ret = get_leaderboard(results)
    print ret
    if ret:
    	parse_leaderboard(results)
    sleep(60 * wait_minutes)
