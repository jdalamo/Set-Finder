//
//  WelcomeViewController.swift
//  Set-Finder
//
//  Created by JD del Alamo on 10/26/23.
//

import UIKit

class WelcomeViewController: UIViewController {
   @IBOutlet weak var continueButton: UIButton!
   @IBOutlet weak var skipWelcomeSwitch: UISwitch!

   override func viewDidLoad() {
      super.viewDidLoad()

      continueButton.addTarget(self, action: #selector(continueButtonTapped), for: .touchUpInside)
      skipWelcomeSwitch.addTarget(self, action: #selector(skipWelcomeSwitchTapped), for: .touchUpInside)
   }

   @objc private func continueButtonTapped(sender: UIButton)
   {
      if let mainController = storyboard?.instantiateViewController(withIdentifier: "main") as? MainViewController {
         present(mainController, animated: true, completion: nil)
      }
   }

   @objc private func skipWelcomeSwitchTapped(sender: UISwitch)
   {
      UserDefaults.standard.set(sender.isOn, forKey: SKIP_WELCOME_KEY)
   }
}
